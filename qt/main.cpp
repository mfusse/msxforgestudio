#include <QApplication>
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QKeySequence>
#include <QMenu>
#include <QPen>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QProcess>
#include <QScrollArea>
#include <QScreen>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QPixmap>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QSignalBlocker>
#include <QTimer>
#include <memory>
#include <cstring>
#include <functional>
#include <vector>
#include "project_io.h"
extern "C" {
#include "msx2_graphics.h"
}

namespace {
const char *tileNames[T_COUNT] = {
    "Césped", "Árbol", "Camino", "Agua", "Muro", "Puerta", "Flores", "Tierra",
    "Techo", "Ladrillos", "Ventana", "Valla", "Jardín", "Pavimento", "Piso", "Mesa",
    "Cama", "Estantería", "Mostrador", "Alfombra", "Silla", "Lámpara", "Armario", "Lavabo",
    "Personalizado 1", "Personalizado 2", "Personalizado 3", "Personalizado 4",
    "Personalizado 5", "Personalizado 6", "Personalizado 7", "Personalizado 8"
};

QImage pixelArt(const uint8_t *pixels, bool transparent) {
    QImage image(16,16,QImage::Format_ARGB32);
    for(int y=0; y<16; ++y) for(int x=0; x<16; ++x) {
        const unsigned int index = pixels[y*16+x];
        if (transparent && index == 255) image.setPixelColor(x,y,Qt::transparent);
        else image.setPixelColor(x,y,QColor::fromRgb(forge_msx2_palette[index & 15]));
    }
    return image;
}
void copyText(char *out, int max, const QString &string) {
    const QByteArray bytes = string.toUtf8();
    forge_copy_string(out,max,bytes.constData());
}
QString txt(const char *bytes) { return QString::fromUtf8(bytes); }

QString style() {
    return QStringLiteral(R"qss(
QMainWindow,QWidget { background:#141E29; color:#DFE9F3; font-family:'Segoe UI'; font-size:10pt; }
QToolBar,QMenuBar,QStatusBar { background:#192634; border:0; spacing:4px; }
QMenu { background:#213247; border:1px solid #38536C; padding:4px; }
QMenu::item { padding:6px 24px; }
QMenu::item:selected { background:#286A9B; }
QDockWidget { color:#90BCE0; font-weight:600; }
QDockWidget::title { background:#203247; padding:7px 11px; text-align:left; }
QTreeWidget,QTextEdit,QLineEdit,QComboBox,QSpinBox,QScrollArea,QTabWidget::pane { background:#1C2C3D; color:#ECF3FA; border:1px solid #354A60; border-radius:4px; padding:4px; }
QTreeWidget::item { min-height:26px; }
QTreeWidget::item:selected { background:#2E608B; color:#FFFFFF; }
QTabBar::tab { background:#203146; padding:8px 18px; margin-right:3px; }
QTabBar::tab:selected { background:#2E618A; border-bottom:2px solid #7CC2F8; }
QTabBar::tab:disabled { color:#697C90; }
QPushButton,QToolButton { color:#E8F2FA; background:#2A4156; border:1px solid #405A72; border-radius:5px; padding:7px 10px; }
QPushButton:hover,QToolButton:hover { background:#376183; }
QPushButton:pressed,QToolButton:checked { background:#2575AA; }
QPushButton:disabled { background:#24303C; color:#778B9D; }
QLabel#help { color:#92B1C8; font-size:9pt; }
QLabel#sectionTitle { color:#7BC2FA; font-weight:700; margin-top:8px; }
QScrollBar:vertical { background:#1D2A39; width:11px; }
QScrollBar::handle:vertical { background:#435D77; border-radius:4px; min-height:30px; }
QScrollBar:horizontal { background:#1D2A39; height:11px; }
QScrollBar::handle:horizontal { background:#435D77; border-radius:4px; min-width:30px; }
)qss");
}

enum class Selection { Scene, NPC, Portal };
enum class Tool { Select, Brush, InsertNPC, InsertDoor };

// Pure canvas: the original C engine remains the only source of project data.
class MapCanvas final : public QWidget {
public:
    ForgeProject *project = nullptr;
    int sceneIndex = 0, selectedNpc = -1, selectedPortal = -1, brushTile = T_GRASS;
    int zoom = 3;
    Tool tool = Tool::Select;
    std::function<void(int,int,Qt::MouseButton)> clicked;

    explicit MapCanvas(QWidget *parent=nullptr): QWidget(parent) {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(16*FW*2,16*FH*2);
    }
    QSize sizeHint() const override { return QSize(FW*16*zoom,FH*16*zoom); }
    void setZoom(int level) {
        zoom = qBound(2,level,7);
        setMinimumSize(sizeHint());
        resize(sizeHint());
        updateGeometry();
        update();
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.fillRect(rect(),QColor("#263344"));
        if (!project || sceneIndex >= project->scene_count) return;
        const ForgeScene &scene = project->scenes[sceneIndex];
        const int step=16*zoom;
        // Cache each distinct tile once per repaint: correct colors come from .msxf art.
        QPixmap tiles[T_COUNT];
        for(int i=0;i<T_COUNT;++i) tiles[i]=QPixmap::fromImage(pixelArt(forge_tile_pixels_const(project,i),false)).scaled(step,step,Qt::IgnoreAspectRatio,Qt::FastTransformation);
        for(int y=0;y<FH;++y) for(int x=0;x<FW;++x)
            painter.drawPixmap(x*step,y*step,tiles[qBound(0,int(scene.map[y*FW+x]),T_COUNT-1)]);
        painter.setPen(QPen(QColor(255,255,255,28),1));
        for(int x=0;x<=FW;++x) painter.drawLine(x*step,0,x*step,FH*step);
        for(int y=0;y<=FH;++y) painter.drawLine(0,y*step,FW*step,y*step);
        for(int k=0;k<scene.portal_count;++k) {
            const ForgePortal &p=scene.portals[k];
            painter.fillRect(p.x*step+step/4,p.y*step+step/4,step/2,step/2,QColor(p.requires_key?"#EDB35C":"#69C5F5"));
            if (k==selectedPortal) {painter.setPen(QPen(QColor("#F7D65F"),3));painter.drawRect(p.x*step+2,p.y*step+2,step-4,step-4);}
        }
        for(int k=0;k<scene.npc_count;++k) {
            const ForgeNPC &n=scene.npcs[k];
            const int kind = qBound(0,int(n.kind),NPC_ART_SLOTS-1);
            painter.drawPixmap(n.x*step,n.y*step,QPixmap::fromImage(pixelArt(project->npc_sprites[kind],true)).scaled(step,step,Qt::IgnoreAspectRatio,Qt::FastTransformation));
            if(k==selectedNpc) {painter.setPen(QPen(QColor("#78C4FF"),3));painter.drawRect(n.x*step+2,n.y*step+2,step-4,step-4);}
        }
        painter.setBrush(QColor("#F6E5A2"));painter.setPen(QColor("#27384B"));
        painter.drawEllipse(scene.spawn_x*step+step/3,scene.spawn_y*step+step/3,step/3,step/3);
    }
    void mousePressEvent(QMouseEvent *e) override {
        if(!project || !clicked) return;
        const QPoint pos=e->position().toPoint();
        const int x=pos.x()/(16*zoom),y=pos.y()/(16*zoom);
        if(x>=0&&x<FW&&y>=0&&y<FH) clicked(x,y,e->button());
    }
    void wheelEvent(QWheelEvent *e) override {
        if(e->modifiers().testFlag(Qt::ControlModifier)) {
            setZoom(zoom+(e->angleDelta().y()>0?1:-1));e->accept();
        }else QWidget::wheelEvent(e);
    }
};

QWidget *sectionWidget(const QString &title, QFormLayout *&form) {
    auto *widget=new QWidget;
    auto *layout=new QVBoxLayout(widget);
    layout->setContentsMargins(12,12,12,12);
    auto *heading=new QLabel(title);heading->setObjectName("sectionTitle");layout->addWidget(heading);
    form=new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setSpacing(12);
    layout->addLayout(form);
    layout->addStretch();
    return widget;
}
}

// Editor nativo de sprites MSX2: edita directamente los 256 píxeles
// de una apariencia en ForgeProject. No modifica el formato .msxf.
class SpriteCanvas final : public QWidget {
public:
    ForgeProject *project=nullptr;
    int slot=0,color=15;
    std::function<void()> beginStroke, changed;
    explicit SpriteCanvas(QWidget *parent=nullptr):QWidget(parent) {
        setMinimumSize(320,320);setMouseTracking(true);
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);p.fillRect(rect(),QColor("#172637"));
        const int side=qMax(1,qMin(width(),height())/16);
        const int ox=(width()-side*16)/2,oy=(height()-side*16)/2;
        if(!project)return;
        for(int y=0;y<16;++y)for(int x=0;x<16;++x){
            const int v=project->npc_sprites[slot][y*16+x];
            QRect r(ox+x*side,oy+y*side,side,side);
            p.fillRect(r,v==255?QColor("#344454"):QColor::fromRgb(forge_msx2_palette[v&15]));
            p.setPen(QColor(105,140,170,85));p.drawRect(r.adjusted(0,0,-1,-1));
        }
    }
    void mousePressEvent(QMouseEvent *e) override {
        if(e->button()!=Qt::LeftButton && e->button()!=Qt::RightButton)return;
        if(beginStroke)beginStroke();paintAt(e->position().toPoint(),e->button()==Qt::RightButton);
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if(e->buttons() & (Qt::LeftButton|Qt::RightButton))
            paintAt(e->position().toPoint(),bool(e->buttons()&Qt::RightButton));
    }
private:
    void paintAt(QPoint at,bool erase){
        if(!project)return;
        const int side=qMax(1,qMin(width(),height())/16);
        const int x=(at.x()-(width()-side*16)/2)/side;
        const int y=(at.y()-(height()-side*16)/2)/side;
        if(at.x()<(width()-side*16)/2||at.y()<(height()-side*16)/2||x<0||y<0||x>=16||y>=16)return;
        auto &v=project->npc_sprites[slot][y*16+x];
        const int requested=erase?255:color;
        if(v!=requested){v=static_cast<uint8_t>(requested);update();if(changed)changed();}
    }
};

class MainWindow final : public QMainWindow {
    std::unique_ptr<ForgeProject> project_ = std::make_unique<ForgeProject>();
    QString currentFile_;
    bool dirty_=false,pendingInspector_=false,inspectorLoading_=false;
    int scene_=0,npc_=-1,portal_=-1,brush_=T_GRASS,appearance_=0,copyNpc_=-1,copyScene_=-1;
    Selection selection_=Selection::Scene;
    Tool tool_=Tool::Select;
    std::vector<ForgeProject> undo_, redo_;

    QDockWidget *explorerDock_=nullptr,*libraryDock_=nullptr,*inspectorDock_=nullptr,*resourceDock_=nullptr,*consoleDock_=nullptr;
    QTreeWidget *tree_=nullptr;
    MapCanvas *map_=nullptr;
    SpriteCanvas *spriteCanvas_=nullptr;
    QTabWidget *centralTabs_=nullptr,*resourceTabs_=nullptr;
    QComboBox *appearancePick_=nullptr,*destPick_=nullptr;
    QLabel *fileLabel_=nullptr,*console_=nullptr,*tileLabel_=nullptr,*appearancePreview_=nullptr;
    std::vector<QToolButton*> terrainButtons_;
    QStackedWidget *inspectorStack_=nullptr;
    QPushButton *apply_=nullptr,*duplicate_=nullptr,*remove_=nullptr,*chooseArrival_=nullptr;
    bool choosingArrival_=false;
    int arrivalSourceScene_=-1, arrivalSourcePortal_=-1, arrivalDestination_=-1;
    bool arrivalRequiresKey_=false;

    QLineEdit *sceneName_=nullptr, *npcName_=nullptr;
    QTextEdit *npcFirst_=nullptr,*npcAgain_=nullptr;
    QComboBox *npcArt_=nullptr,*npcBehavior_=nullptr,*npcSpeed_=nullptr,*npcEvent_=nullptr,*npcFlag_=nullptr,*npcReward_=nullptr;
    QSpinBox *npcRadius_=nullptr,*portalX_=nullptr,*portalY_=nullptr;
    QCheckBox *npcKey_=nullptr,*portalKey_=nullptr,*portalAuto_=nullptr,*npcFlagSet_=nullptr;
    QLineEdit *choiceQuestion_=nullptr;
    QComboBox *portalDest_=nullptr;

    void log(const QString &message) {
        console_->setText(message);statusBar()->showMessage(message,5000);
    }
    void refreshArt() {
        if(spriteCanvas_)spriteCanvas_->update();
        for(size_t i=0;i<terrainButtons_.size();++i)
            terrainButtons_[i]->setIcon(QIcon(QPixmap::fromImage(pixelArt(forge_tile_pixels_const(project_.get(),int(i)),false))
                                                     .scaled(38,38,Qt::IgnoreAspectRatio,Qt::FastTransformation)));
        if(appearancePreview_&&appearance_>=0&&appearance_<NPC_ART_SLOTS)
            appearancePreview_->setPixmap(QPixmap::fromImage(pixelArt(project_->npc_sprites[appearance_],true))
                                                  .scaled(80,80,Qt::KeepAspectRatio,Qt::FastTransformation));
    }
    void title() {
        const QString file=currentFile_.isEmpty()?QStringLiteral("Sin título.msxf"):QFileInfo(currentFile_).fileName();
        setWindowTitle(QStringLiteral("%1%2 — MSX Forge Studio 0.6.0 α").arg(file,dirty_||pendingInspector_?" *":""));
        if(fileLabel_) fileLabel_->setText(file+(dirty_||pendingInspector_?"  • Sin guardar":""));
    }
    void changed() { dirty_=true;title(); }
    void markPending() {if(!inspectorLoading_) {pendingInspector_=true; title();}}
    void snapshot() {undo_.push_back(*project_);if(undo_.size()>24)undo_.erase(undo_.begin());redo_.clear();}
    ForgeScene &activeScene() {return project_->scenes[scene_];}
    const ForgeScene &activeScene() const {return project_->scenes[scene_];}

    void cancelArrival(bool restore=true) {
        if(!choosingArrival_)return;
        choosingArrival_=false;
        int source=arrivalSourceScene_,portal=arrivalSourcePortal_;
        arrivalSourceScene_=arrivalSourcePortal_=arrivalDestination_=-1;
        if(restore && source>=0 && source<project_->scene_count) {
            switchScene(source);
            if(portal>=0 && portal<activeScene().portal_count)select(Selection::Portal,portal);
        }
        log("Selección de llegada cancelada; la puerta no cambió.");
    }
    void beginArrival() {
        if(selection_!=Selection::Portal || portal_<0 || choosingArrival_)return;
        const int dest=portalDest_->currentIndex();
        if(dest<0 || dest>=project_->scene_count) {log("Elegí primero un escenario de destino.");return;}
        arrivalSourceScene_=scene_;arrivalSourcePortal_=portal_;
        arrivalDestination_=dest;arrivalRequiresKey_=portalKey_->isChecked();
        choosingArrival_=true;
        // The draft destination and key are captured; no premature apply/dirty save.
        pendingInspector_=false;title();
        scene_=dest;selection_=Selection::Scene;npc_=portal_=-1;
        map_->sceneIndex=dest;map_->selectedNpc=map_->selectedPortal=-1;
        updateInspector();rebuildTree();map_->update();
        log("ELEGIR LLEGADA: hacé clic en una casilla caminable libre. Clic derecho cancela.");
    }
    void chooseTool(Tool tool) {tool_=tool;map_->tool=tool;if(tool!=Tool::InsertNPC){copyNpc_=-1;copyScene_=-1;}}
    void pick(int x,int y,Qt::MouseButton button) {
        if(choosingArrival_) {
            if(button==Qt::RightButton){cancelArrival();return;}
            if(button!=Qt::LeftButton)return;
            const ForgeScene &dest=activeScene();
            bool occupied=(dest.spawn_x==x && dest.spawn_y==y);
            for(int i=0;i<dest.npc_count;++i)occupied |= dest.npcs[i].x==x && dest.npcs[i].y==y;
            for(int i=0;i<dest.portal_count;++i)occupied |= dest.portals[i].x==x && dest.portals[i].y==y;
            if(occupied || !forge_tile_walkable(&dest,x,y)) {
                log("Llegada inválida: elegí una casilla caminable sin NPC, puerta ni inicio.");return;
            }
            const int source=arrivalSourceScene_,portal=arrivalSourcePortal_;
            if(source<0 || source>=project_->scene_count || portal<0 || portal>=project_->scenes[source].portal_count){cancelArrival(false);return;}
            snapshot();ForgePortal &p=project_->scenes[source].portals[portal];
            p.to=uint8_t(arrivalDestination_);p.tx=uint8_t(x);p.ty=uint8_t(y);
            p.requires_key=uint8_t(arrivalRequiresKey_);
            choosingArrival_=false;
            arrivalSourceScene_=arrivalSourcePortal_=arrivalDestination_=-1;
            scene_=source;selection_=Selection::Scene;npc_=portal_=-1;
            map_->sceneIndex=source;map_->selectedNpc=map_->selectedPortal=-1;
            changed();select(Selection::Portal,portal);
            log(QString("Llegada guardada: casilla (%1,%2). ").arg(x).arg(y)+"La puerta conserva su condición de llave.");
            return;
        }
        if(button==Qt::RightButton && copyNpc_>=0) {copyNpc_=-1;copyScene_=-1;log("Duplicación cancelada.");return;}
        if(button==Qt::RightButton || tool_==Tool::Select) {
            const ForgeScene &sc=activeScene();
            for(int i=0;i<sc.npc_count;++i) if(sc.npcs[i].x==x&&sc.npcs[i].y==y) {select(Selection::NPC,i);return;}
            for(int i=0;i<sc.portal_count;++i) if(sc.portals[i].x==x&&sc.portals[i].y==y) {select(Selection::Portal,i);return;}
            select(Selection::Scene,-1);return;
        }
        ForgeScene &sc=activeScene();
        if (tool_==Tool::Brush) {
            if (brush_==T_DOOR) {log("Usá la herramienta Puerta para crear conexiones válidas.");return;}
            if (sc.map[y*FW+x]==brush_)return;
            bool protectedTile=(sc.spawn_x==x&&sc.spawn_y==y);
            for(int i=0;i<sc.npc_count;++i)protectedTile|=(sc.npcs[i].x==x&&sc.npcs[i].y==y);
            for(int i=0;i<sc.portal_count;++i)protectedTile|=(sc.portals[i].x==x&&sc.portals[i].y==y);
            if(protectedTile){log("Esta casilla contiene el inicio, un NPC o una puerta. No se puede pintar encima.");return;}
            snapshot();sc.map[y*FW+x]=uint8_t(brush_);changed();map_->update();return;
        }
        bool occupied=(sc.spawn_x==x&&sc.spawn_y==y);
        for(int i=0;i<sc.npc_count;++i) occupied|=sc.npcs[i].x==x&&sc.npcs[i].y==y;
        for(int i=0;i<sc.portal_count;++i) occupied|=sc.portals[i].x==x&&sc.portals[i].y==y;
        if (occupied || forge_tile_blocking(sc.map[y*FW+x])) {log("Elegí una casilla libre y caminable.");return;}
        if(tool_==Tool::InsertNPC) {
            if(sc.npc_count>=MAX_NPCS) {log("Se alcanzó el límite de 8 NPC en el escenario.");return;}
            if(copyNpc_>=0) {
                if(copyScene_!=scene_) {copyNpc_=-1;copyScene_=-1;log("Solo se puede duplicar en el mismo escenario.");return;}
                snapshot();int newIndex=forge_duplicate_npc(project_.get(),scene_,copyNpc_,x,y);
                if(newIndex<0){undo_.pop_back();log("No se pudo duplicar en esa casilla.");return;}
                copyNpc_=-1;copyScene_=-1;
                changed();rebuildTree();select(Selection::NPC,newIndex);
                log("NPC duplicado; modificá su nombre y diálogo en el Inspector.");return;
            }
            snapshot();const int index=sc.npc_count++;
            ForgeNPC &n=sc.npcs[index];std::memset(&n,0,sizeof(n));
            n.x=uint8_t(x);n.y=uint8_t(y);n.kind=uint8_t(appearance_);
            forge_copy_string(n.name,sizeof(n.name),"NPC nuevo");
            forge_copy_string(n.first,sizeof(n.first),"Hola, viajero.");
            forge_copy_string(n.again,sizeof(n.again),"Hasta luego!");
            project_->npc_behavior[scene_][index]=NPC_STILL;
            project_->npc_speed[scene_][index]=NPC_NORMAL;
            project_->npc_radius[scene_][index]=1;
            changed();rebuildTree();select(Selection::NPC,index);
        } else if(tool_==Tool::InsertDoor) {
            if(sc.portal_count>=MAX_PORTALS) {log("Se alcanzó el límite de 8 puertas.");return;}
            if(project_->scene_count<=1){log("Creá otro escenario antes de añadir una puerta.");return;}
            int dest=destPick_->currentData().toInt();
            if(dest<0||dest>=project_->scene_count){log("Seleccioná un escenario de destino.");return;}
            snapshot();const int index=sc.portal_count++;
            ForgePortal &p=sc.portals[index];std::memset(&p,0,sizeof(p));
            p.x=uint8_t(x);p.y=uint8_t(y);p.to=uint8_t(dest);
            p.tx=project_->scenes[dest].spawn_x;p.ty=project_->scenes[dest].spawn_y;
            sc.map[y*FW+x]=T_DOOR;
            changed();rebuildTree();select(Selection::Portal,index);
        }
    }

    bool processPending() {
        if(!pendingInspector_)return true;
        QMessageBox msg(this);msg.setIcon(QMessageBox::Question);
        msg.setWindowTitle("Cambios pendientes en el inspector");
        msg.setText("Modificaste propiedades que todavía no aplicaste.");
        auto *apply=msg.addButton("Aplicar cambios",QMessageBox::AcceptRole);
        msg.addButton("Descartar cambios",QMessageBox::DestructiveRole);
        auto *cancel=msg.addButton("Cancelar",QMessageBox::RejectRole);
        msg.exec();if(msg.clickedButton()==cancel)return false;
        if(msg.clickedButton()==apply)return applyInspector();
        pendingInspector_=false;title();return true;
    }
    void select(Selection which,int index) {
        if(which==selection_&&((which==Selection::NPC&&index==npc_)||(which==Selection::Portal&&index==portal_)||(which==Selection::Scene&&npc_<0&&portal_<0)))return;
        if(choosingArrival_) {log("Marcá primero la casilla de llegada en el mapa.");return;}
        if(!processPending()) {rebuildTree();return;}
        selection_=which;npc_=which==Selection::NPC?index:-1;portal_=which==Selection::Portal?index:-1;
        updateInspector();rebuildTree();map_->selectedNpc=npc_;map_->selectedPortal=portal_;map_->update();
    }
    void switchScene(int index) {
        if(choosingArrival_) {log("Terminá la selección de llegada o cancelá con clic derecho.");return;}
        if(index<0||index>=project_->scene_count)return;
        if(index!=scene_&&!processPending()){rebuildTree();return;}
        scene_=index;selection_=Selection::Scene;npc_=portal_=-1;copyNpc_=copyScene_=-1;
        map_->sceneIndex=index;map_->selectedNpc=map_->selectedPortal=-1;
        updateInspector();rebuildTree();map_->update();
    }
    void rebuildTree() {
        QSignalBlocker blocker(tree_);
        QSignalBlocker noDestSignal(destPick_);
        QSignalBlocker noPortalSignal(portalDest_);
        tree_->clear();
        for(int s=0;s<project_->scene_count;++s) {
            const ForgeScene &scene=project_->scenes[s];
            auto *root=new QTreeWidgetItem(tree_,QStringList()<<txt(scene.name)+QString("  (%1/%2 NPC)").arg(scene.npc_count).arg(MAX_NPCS));
            root->setData(0,Qt::UserRole,0);root->setData(0,Qt::UserRole+1,s);
            for(int n=0;n<scene.npc_count;++n){
                auto *item=new QTreeWidgetItem(root,QStringList()<<QStringLiteral("♟ ")+txt(scene.npcs[n].name));
                item->setData(0,Qt::UserRole,1);item->setData(0,Qt::UserRole+1,s);item->setData(0,Qt::UserRole+2,n);
                if(s==scene_&&selection_==Selection::NPC&&npc_==n) tree_->setCurrentItem(item);
            }
            for(int n=0;n<scene.portal_count;++n){
                auto *item=new QTreeWidgetItem(root,QStringList()<<QStringLiteral("↳ Puerta → ")+txt(project_->scenes[qBound(0,int(scene.portals[n].to),int(project_->scene_count)-1)].name));
                item->setData(0,Qt::UserRole,2);item->setData(0,Qt::UserRole+1,s);item->setData(0,Qt::UserRole+2,n);
                if(s==scene_&&selection_==Selection::Portal&&portal_==n) tree_->setCurrentItem(item);
            }
            root->setExpanded(s==scene_);
            if(s==scene_&&selection_==Selection::Scene)tree_->setCurrentItem(root);
        }
        destPick_->clear();portalDest_->clear();
        for(int s=0;s<project_->scene_count;++s){
            destPick_->addItem(txt(project_->scenes[s].name),s);
            portalDest_->addItem(txt(project_->scenes[s].name),s);
        }
        destPick_->setCurrentIndex(qMin(scene_+1,int(project_->scene_count)-1));
        if(selection_==Selection::Portal&&portal_>=0)portalDest_->setCurrentIndex(activeScene().portals[portal_].to);
    }

    void updateInspector() {
        inspectorLoading_=true;pendingInspector_=false;
        if(selection_==Selection::NPC&&npc_>=0&&npc_<activeScene().npc_count){
            const ForgeNPC &n=activeScene().npcs[npc_];
            npcName_->setText(txt(n.name));npcFirst_->setPlainText(txt(n.first));npcAgain_->setPlainText(txt(n.again));
            npcArt_->setCurrentIndex(n.kind);npcKey_->setChecked(n.gives_key);
            npcBehavior_->setCurrentIndex(project_->npc_behavior[scene_][npc_]);
            npcSpeed_->setCurrentIndex(project_->npc_speed[scene_][npc_]);
            npcRadius_->setValue(project_->npc_radius[scene_][npc_]);
            npcEvent_->setCurrentIndex(project_->npc_event_mode[scene_][npc_]);
            choiceQuestion_->setText(txt(project_->npc_choice_question[scene_][npc_]));
            npcFlag_->setCurrentIndex(project_->npc_flag_index[scene_][npc_]);
            npcFlagSet_->setChecked(project_->npc_choice_set_flag[scene_][npc_]);
            npcReward_->setCurrentIndex(project_->npc_reward[scene_][npc_]);
            inspectorStack_->setCurrentIndex(1);
        }else if(selection_==Selection::Portal&&portal_>=0&&portal_<activeScene().portal_count){
            const ForgePortal &p=activeScene().portals[portal_];
            portalDest_->setCurrentIndex(p.to);
            const bool automatic = p.tx==255 && p.ty==255;
            portalAuto_->setChecked(automatic);
            portalX_->setValue(automatic?0:p.tx);portalY_->setValue(automatic?0:p.ty);
            portalX_->setEnabled(!automatic);portalY_->setEnabled(!automatic);
            portalKey_->setChecked(p.requires_key);
            inspectorStack_->setCurrentIndex(2);
        }else{
            sceneName_->setText(txt(activeScene().name));inspectorStack_->setCurrentIndex(0);
        }
        chooseArrival_->setVisible(selection_==Selection::Portal && portal_>=0);
        apply_->setText("Aplicar cambios");
        duplicate_->setEnabled(selection_==Selection::NPC&&activeScene().npc_count<MAX_NPCS);
        remove_->setEnabled((selection_==Selection::NPC&&npc_>=0)||(selection_==Selection::Portal&&portal_>=0));
        inspectorLoading_=false;title();
    }
    bool applyInspector() {
        if(!pendingInspector_)return true;
        if(selection_==Selection::NPC&&npc_>=0){
            snapshot();ForgeNPC &n=activeScene().npcs[npc_];
            copyText(n.name,sizeof(n.name),npcName_->text());
            copyText(n.first,sizeof(n.first),npcFirst_->toPlainText());
            copyText(n.again,sizeof(n.again),npcAgain_->toPlainText());
            n.kind=uint8_t(npcArt_->currentIndex());n.gives_key=uint8_t(npcKey_->isChecked());
            project_->npc_behavior[scene_][npc_]=uint8_t(npcBehavior_->currentIndex());
            project_->npc_speed[scene_][npc_]=uint8_t(npcSpeed_->currentIndex());
            project_->npc_radius[scene_][npc_]=uint8_t(npcRadius_->value());
            project_->npc_event_mode[scene_][npc_]=uint8_t(npcEvent_->currentIndex());
            copyText(project_->npc_choice_question[scene_][npc_],64,choiceQuestion_->text());
            project_->npc_flag_index[scene_][npc_]=uint8_t(npcFlag_->currentIndex());
            project_->npc_choice_set_flag[scene_][npc_]=uint8_t(npcFlagSet_->isChecked());
            project_->npc_reward[scene_][npc_]=uint8_t(npcReward_->currentIndex());
        }else if(selection_==Selection::Portal&&portal_>=0){
            const int dest=portalDest_->currentIndex(), x=portalX_->value(), y=portalY_->value();
            if(dest<0||dest>=project_->scene_count) {log("Destino de puerta inválido.");return false;}
            const bool automatic=portalAuto_->isChecked();
            bool occupied=false;
            for(int i=0;!automatic && i<project_->scenes[dest].portal_count;++i)
                occupied|=(project_->scenes[dest].portals[i].x==x&&project_->scenes[dest].portals[i].y==y);
            const ForgeScene &target=project_->scenes[dest];
            if(!automatic){
                occupied|=(target.spawn_x==x && target.spawn_y==y);
                for(int i=0;i<target.npc_count;++i)occupied|=(target.npcs[i].x==x && target.npcs[i].y==y);
            }
            if(!automatic && (occupied||!forge_tile_walkable(&target,x,y))){
                QMessageBox::warning(this,"Llegada inválida","La casilla de llegada no está libre o es inaccesible.");return false;
            }
            snapshot();ForgePortal &p=activeScene().portals[portal_];
            p.to=uint8_t(dest);p.tx=automatic?255:uint8_t(x);p.ty=automatic?255:uint8_t(y);p.requires_key=uint8_t(portalKey_->isChecked());
        }else{
            if(sceneName_->text().trimmed().isEmpty()){log("El escenario debe tener nombre.");return false;}
            snapshot();copyText(activeScene().name,sizeof(activeScene().name),sceneName_->text());
        }
        pendingInspector_=false;changed();rebuildTree();map_->update();log("Propiedades aplicadas.");return true;
    }
    bool save(bool as=false) {
        if(choosingArrival_){log("Completá o cancelá la selección de llegada antes de guardar.");return false;}
        if(pendingInspector_&&!applyInspector())return false;
        QString path=currentFile_;
        if(as||path.isEmpty())path=QFileDialog::getSaveFileName(this,"Guardar proyecto MSX Forge",path,"Proyecto MSX (*.msxf)");
        if(path.isEmpty())return false;
        if(!path.endsWith(".msxf",Qt::CaseInsensitive))path+=".msxf";
        QString error;
        if(!ProjectIO::save(path,*project_,error)) {QMessageBox::critical(this,"No se pudo guardar",error);return false;}
        currentFile_=path;dirty_=false;title();log("Proyecto guardado sin errores.");return true;
    }
    bool guardChanges() {
        if(!dirty_&&!pendingInspector_)return true;
        auto result=QMessageBox::warning(this,"Cambios sin guardar","El proyecto tiene cambios pendientes. ¿Querés guardarlos?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
        if(result==QMessageBox::Cancel)return false;
        if(result==QMessageBox::Save)return save();
        return true;
    }
    void loadProject() {
        if(choosingArrival_){log("Cancelá la selección de llegada antes de abrir otro proyecto.");return;}
        const QString path=QFileDialog::getOpenFileName(this,"Abrir proyecto MSX Forge",QString(),"Proyectos MSX (*.msxf)");
        if(path.isEmpty())return;
        auto next=std::make_unique<ForgeProject>();QString error;
        if(!ProjectIO::load(path,*next,error)) {QMessageBox::critical(this,"Proyecto inválido",error);return;}
        if(!guardChanges())return;
        project_=std::move(next);currentFile_=path;dirty_=pendingInspector_=false;undo_.clear();redo_.clear();scene_=0;selection_=Selection::Scene;npc_=portal_=-1;copyNpc_=copyScene_=-1;
        map_->project=project_.get();map_->sceneIndex=0;
        rebuildTree();updateInspector();refreshArt();map_->update();log("Proyecto compatible abierto.");
    }
    void newProject(bool demo=false) {
        if(choosingArrival_){log("Cancelá la selección de llegada antes de crear otro proyecto.");return;}
        if(!guardChanges())return;
        if(demo)forge_demo(project_.get());else forge_blank(project_.get());
        currentFile_.clear();dirty_=pendingInspector_=false;undo_.clear();redo_.clear();scene_=0;selection_=Selection::Scene;npc_=portal_=-1;copyNpc_=copyScene_=-1;
        rebuildTree();updateInspector();refreshArt();map_->update();title();log(demo?"Demo cargada.":"Nuevo proyecto creado.");
    }
    void exportRom(bool test) {
        if(choosingArrival_){log("Elegí una casilla de llegada antes de exportar la ROM.");return;}
        if(pendingInspector_&&!applyInspector())return;
        QString path;
        if(test){path=QDir::temp().filePath("MSXForge_060_prueba.rom");}
        else path=QFileDialog::getSaveFileName(this,"Exportar cartucho MSX",QString(),"Cartucho MSX (*.rom)");
        if(path.isEmpty())return;
        if(!path.endsWith(".rom",Qt::CaseInsensitive))path+=".rom";
        QString error;
        if(!ProjectIO::exportRom(path,*project_,error)){QMessageBox::critical(this,"ROM inválida",error);return;}
        log("ROM de 32 KB exportada: "+path);
        if(!test)return;
        QSettings settings("MSXForge","Studio060");
        QString exe=settings.value("openmsxPath",QStandardPaths::findExecutable("openmsx")).toString();
        if(exe.isEmpty()||(!QFileInfo::exists(exe)&&!exe.contains("openmsx")))exe=QFileDialog::getOpenFileName(this,"Elegí openmsx.exe",QString(),"openMSX (*.exe);;Programas (*)");
        if(exe.isEmpty())return;
        const QString machine=project_->profile==PROFILE_MSX?"C-BIOS_MSX1":"C-BIOS_MSX2+";
        if(!QProcess::startDetached(exe,{"-machine",machine,"-cart",path})){
            exe=QFileDialog::getOpenFileName(this,"No encontré openMSX: seleccioná el ejecutable",QString(),"openMSX (*.exe);;Programas (*)");
            if(exe.isEmpty()||!QProcess::startDetached(exe,{"-machine",machine,"-cart",path})){
                QMessageBox::warning(this,"No pude iniciar openMSX","La ROM se generó correctamente, pero no se pudo iniciar el emulador.");return;
            }
        }
        settings.setValue("openmsxPath",exe);log("ROM cargada en openMSX.");
    }
    void undo(bool redo=false) {
        if(choosingArrival_){cancelArrival();return;}
        if(!processPending())return;
        auto &from=redo?redo_:undo_;auto &to=redo?undo_:redo_;
        if(from.empty())return;
        to.push_back(*project_);*project_=from.back();from.pop_back();
        scene_=qMin(scene_,int(project_->scene_count)-1);selection_=Selection::Scene;npc_=portal_=-1;
        rebuildTree();updateInspector();refreshArt();map_->sceneIndex=scene_;map_->update();changed();
    }
    void duplicateNpc() {
        if(selection_!=Selection::NPC||npc_<0||!processPending())return;
        if(activeScene().npc_count>=MAX_NPCS){log("Límite de 8 NPC en el escenario.");return;}
        copyNpc_=npc_;copyScene_=scene_;tool_=Tool::InsertNPC;map_->tool=tool_;
        log("Duplicar: marcá una casilla libre en el mapa. Clic derecho cancela.");
    }
    void removeSelected() {
        if(selection_==Selection::Scene)return;
        if(!processPending())return;
        if(QMessageBox::question(this,"Eliminar objeto","¿Eliminar el objeto seleccionado?")!=QMessageBox::Yes)return;
        snapshot();
        if(selection_==Selection::NPC)forge_delete_npc(project_.get(),scene_,npc_);
        else if(selection_==Selection::Portal){
            ForgeScene &sc=activeScene();ForgePortal &p=sc.portals[portal_];
            sc.map[p.y*FW+p.x]=T_PATH;
            for(int i=portal_;i+1<sc.portal_count;++i)sc.portals[i]=sc.portals[i+1];
            --sc.portal_count;
        }
        selection_=Selection::Scene;npc_=portal_=-1;rebuildTree();updateInspector();map_->update();changed();
    }
    void addScene() {
        if(!processPending())return;
        if(project_->scene_count>=MAX_SCENES){QMessageBox::information(this,"Límite de MSX","El motor admite hasta 16 escenarios.");return;}
        const QString name=QStringLiteral("Escenario %1").arg(project_->scene_count+1);
        snapshot();const int index=project_->scene_count++;
        ForgeScene &scene=project_->scenes[index];std::memset(&scene,0,sizeof(scene));
        copyText(scene.name,sizeof(scene.name),name);
        scene.spawn_x=2;scene.spawn_y=2;
        for(int y=0;y<FH;++y)for(int x=0;x<FW;++x)scene.map[y*FW+x]=(x==0||y==0||x==FW-1||y==FH-1)?T_TREE:T_GRASS;
        for(int i=0;i<MAX_NPCS;++i){project_->npc_behavior[index][i]=NPC_STILL;project_->npc_speed[index][i]=NPC_NORMAL;project_->npc_radius[index][i]=1;}
        changed();switchScene(index);
    }
    void defaultDocking() {
        for(auto *dock:{explorerDock_,libraryDock_,inspectorDock_,resourceDock_,consoleDock_}) {removeDockWidget(dock);dock->show();dock->setFloating(false);}
        addDockWidget(Qt::LeftDockWidgetArea,explorerDock_);
        addDockWidget(Qt::LeftDockWidgetArea,libraryDock_);
        splitDockWidget(explorerDock_,libraryDock_,Qt::Vertical);
        addDockWidget(Qt::RightDockWidgetArea,inspectorDock_);
        addDockWidget(Qt::BottomDockWidgetArea,resourceDock_);
        addDockWidget(Qt::BottomDockWidgetArea,consoleDock_);
        tabifyDockWidget(resourceDock_,consoleDock_);
        resourceDock_->raise();
        resizeDocks({explorerDock_,inspectorDock_},{235,320},Qt::Horizontal);
        resizeDocks({resourceDock_},{170},Qt::Vertical);
    }
    void recoverFloatingDocks() {
        const auto screens=QGuiApplication::screens();
        for(auto *dock:{explorerDock_,libraryDock_,inspectorDock_,resourceDock_,consoleDock_}) {
            if(!dock->isFloating())continue;
            bool visible=false;
            for(QScreen *screen:screens) if(screen->availableGeometry().intersects(dock->geometry())){visible=true;break;}
            if(!visible){dock->setFloating(false);addDockWidget(Qt::RightDockWidgetArea,dock);}
        }
    }
    void makeInspector() {
        auto *content=new QWidget;auto *outer=new QVBoxLayout(content);
        outer->setContentsMargins(0,0,0,0);
        inspectorStack_=new QStackedWidget;
        QFormLayout *form=nullptr;
        QWidget *scenePage=sectionWidget("ESCENARIO",form);
        sceneName_=new QLineEdit;sceneName_->setMaxLength(31);form->addRow("Nombre",sceneName_);
        form->addRow(new QLabel("16 × 12 casillas · MSX2 · 16 escenas máximo"));
        inspectorStack_->addWidget(scenePage);
        QWidget *npcPage=sectionWidget("PERSONAJE / NPC",form);
        npcName_=new QLineEdit;npcName_->setMaxLength(27);form->addRow("Nombre",npcName_);
        npcArt_=new QComboBox;form->addRow("Apariencia",npcArt_);
        for(int i=0;i<NPC_ART_SLOTS;++i)npcArt_->addItem(QString("Apariencia %1").arg(i+1));
        npcFirst_=new QTextEdit;npcFirst_->setFixedHeight(90);npcFirst_->setToolTip("Máximo 77 bytes UTF-8 en la ROM.");form->addRow("Primer diálogo",npcFirst_);
        npcAgain_=new QTextEdit;npcAgain_->setFixedHeight(90);npcAgain_->setToolTip("Máximo 77 bytes UTF-8 en la ROM.");form->addRow("Diálogo posterior",npcAgain_);
        npcKey_=new QCheckBox("Entrega llave");form->addRow("Objeto clave",npcKey_);
        npcBehavior_=new QComboBox;npcBehavior_->addItems({"Quieto","Libre","Zona cercana"});form->addRow("Movimiento",npcBehavior_);
        npcSpeed_=new QComboBox;npcSpeed_->addItems({"Lento","Normal","Rápido"});form->addRow("Velocidad",npcSpeed_);
        npcRadius_=new QSpinBox;npcRadius_->setRange(1,4);form->addRow("Radio",npcRadius_);
        npcEvent_=new QComboBox;npcEvent_->addItems({"Clásico","Condición de llave","Elección SÍ/NO","Estado de misión"});form->addRow("Evento",npcEvent_);
        choiceQuestion_=new QLineEdit;choiceQuestion_->setMaxLength(63);form->addRow("Pregunta",choiceQuestion_);
        npcFlag_=new QComboBox;npcFlag_->addItems({"Misión 1","Misión 2","Misión 3","Misión 4"});form->addRow("Estado",npcFlag_);
        npcFlagSet_=new QCheckBox("Marcar si responde SÍ");form->addRow("Acción SÍ",npcFlagSet_);
        npcReward_=new QComboBox;npcReward_->addItems({"Sin recompensa","5 monedas","10 monedas","20 monedas"});form->addRow("Recompensa",npcReward_);
        inspectorStack_->addWidget(npcPage);
        QWidget *portalPage=sectionWidget("PUERTA / PORTAL",form);
        portalDest_=new QComboBox;form->addRow("Destino",portalDest_);
        portalAuto_=new QCheckBox("Retorno automático (interiores)");form->addRow("Regreso",portalAuto_);
        portalX_=new QSpinBox;portalX_->setRange(0,FW-1);form->addRow("Llegada X",portalX_);
        portalY_=new QSpinBox;portalY_->setRange(0,FH-1);form->addRow("Llegada Y",portalY_);
        portalKey_=new QCheckBox("Requiere llave");form->addRow("Acceso",portalKey_);
        chooseArrival_=new QPushButton("⌖ Elegir llegada en el mapa");
        chooseArrival_->setToolTip("Elegí visualmente la casilla de llegada en el escenario destino; clic derecho para cancelar.");
        form->addRow(chooseArrival_);
        form->addRow(new QLabel("En el mapa destino, marcá una casilla libre y caminable."));
        inspectorStack_->addWidget(portalPage);
        auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);scroll->setWidget(inspectorStack_);
        outer->addWidget(scroll,1);
        auto *footer=new QWidget;auto *buttons=new QHBoxLayout(footer);buttons->setContentsMargins(9,9,9,9);
        apply_=new QPushButton("Aplicar cambios");apply_->setObjectName("primary");
        duplicate_=new QPushButton("Duplicar");remove_=new QPushButton("Eliminar");
        buttons->addWidget(apply_,2);buttons->addWidget(duplicate_,1);buttons->addWidget(remove_,1);
        outer->addWidget(footer,0);
        inspectorDock_=new QDockWidget("Inspector",this);inspectorDock_->setObjectName("DockInspector");inspectorDock_->setWidget(content);
        connect(apply_,&QPushButton::clicked,this,[this]{applyInspector();});
        connect(chooseArrival_,&QPushButton::clicked,this,[this]{beginArrival();});
        connect(duplicate_,&QPushButton::clicked,this,[this]{duplicateNpc();});
        connect(remove_,&QPushButton::clicked,this,[this]{removeSelected();});
        connect(sceneName_,&QLineEdit::textEdited,this,[this]{markPending();});
        connect(npcName_,&QLineEdit::textEdited,this,[this]{markPending();});
        connect(choiceQuestion_,&QLineEdit::textEdited,this,[this]{markPending();});
        connect(npcFirst_,&QTextEdit::textChanged,this,[this]{markPending();});
        connect(npcAgain_,&QTextEdit::textChanged,this,[this]{markPending();});
        for(auto *combo:{npcArt_,npcBehavior_,npcSpeed_,npcEvent_,npcFlag_,npcReward_,portalDest_})
            connect(combo,&QComboBox::currentIndexChanged,this,[this](int){markPending();});
        for(auto *check:{npcKey_,npcFlagSet_,portalKey_})connect(check,&QCheckBox::toggled,this,[this](bool){markPending();});
        connect(portalAuto_,&QCheckBox::toggled,this,[this](bool automatic){
            portalX_->setEnabled(!automatic);portalY_->setEnabled(!automatic);markPending();
        });
        for(auto *spin:{npcRadius_,portalX_,portalY_})connect(spin,&QSpinBox::valueChanged,this,[this](int){markPending();});
    }
    void makeExplorer() {
        tree_=new QTreeWidget;tree_->setHeaderHidden(true);tree_->setIndentation(16);
        tree_->setAlternatingRowColors(false);
        explorerDock_=new QDockWidget("Escenarios",this);explorerDock_->setObjectName("DockEscenas");
        auto *content=new QWidget;auto *layout=new QVBoxLayout(content);layout->setContentsMargins(6,6,6,6);
        auto *add=new QPushButton("+ Escenario");layout->addWidget(add);layout->addWidget(tree_,1);explorerDock_->setWidget(content);
        connect(add,&QPushButton::clicked,this,[this]{addScene();});
        connect(tree_,&QTreeWidget::itemClicked,this,[this](QTreeWidgetItem *item,int){
            if(!item)return;const int s=item->data(0,Qt::UserRole+1).toInt();
            const int kind=item->data(0,Qt::UserRole).toInt();
            if(scene_!=s){switchScene(s);if(scene_!=s)return;}
            if(kind==0)select(Selection::Scene,-1);
            if(kind==1)select(Selection::NPC,item->data(0,Qt::UserRole+2).toInt());
            if(kind==2)select(Selection::Portal,item->data(0,Qt::UserRole+2).toInt());
        });
    }
    void makeLibrary() {
        libraryDock_=new QDockWidget("Biblioteca de NPC",this);libraryDock_->setObjectName("DockBiblioteca");
        auto *content=new QWidget;auto *layout=new QVBoxLayout(content);
        auto *hint=new QLabel("Elegí apariencia y colocá un NPC en el mapa.");hint->setWordWrap(true);hint->setObjectName("help");layout->addWidget(hint);
        appearancePick_=new QComboBox;
        for(int i=0;i<NPC_ART_SLOTS;++i)appearancePick_->addItem(QString("Personaje %1").arg(i+1),i);
        layout->addWidget(appearancePick_);
        appearancePreview_=new QLabel;
        appearancePreview_->setMinimumHeight(88);
        appearancePreview_->setAlignment(Qt::AlignCenter);
        layout->addWidget(appearancePreview_);
        connect(appearancePick_,&QComboBox::currentIndexChanged,this,[this](int idx){
            if(idx<0)return;appearance_=idx;refreshArt();
        });
        layout->addStretch();
        libraryDock_->setWidget(content);
        QTimer::singleShot(0,this,[this]{refreshArt();});
    }
    void makeResources() {
        resourceDock_=new QDockWidget("Terrenos",this);resourceDock_->setObjectName("DockTerrenos");
        auto *content=new QWidget;auto *layout=new QVBoxLayout(content);layout->setContentsMargins(6,6,6,6);
        tileLabel_=new QLabel("Pincel: Césped");layout->addWidget(tileLabel_);
        auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);
        auto *tilePanel=new QWidget;auto *tiles=new QGridLayout(tilePanel);tiles->setSpacing(5);
        for(int i=0;i<T_COUNT;++i){
            const int tile=i;auto *btn=new QToolButton;
            btn->setToolTip(QString::fromUtf8(tileNames[i]));
            btn->setIcon(QIcon(QPixmap::fromImage(pixelArt(forge_tile_pixels_const(project_.get(),i),false)).scaled(38,38,Qt::IgnoreAspectRatio,Qt::FastTransformation)));
            btn->setIconSize(QSize(38,38));btn->setMinimumSize(50,50);btn->setCheckable(true);
            if(i==T_GRASS)btn->setChecked(true);
            tiles->addWidget(btn,i/8,i%8);
            terrainButtons_.push_back(btn);
            connect(btn,&QToolButton::clicked,this,[this,btn,tile,tiles]{
                for(int k=0;k<tiles->count();++k)if(auto *other=qobject_cast<QToolButton*>(tiles->itemAt(k)->widget()))other->setChecked(other==btn);
                brush_=tile;map_->brushTile=tile;tileLabel_->setText(QString("Pincel: %1").arg(QString::fromUtf8(tileNames[tile])));
                chooseTool(tile==T_DOOR?Tool::InsertDoor:Tool::Brush);
            });
        }
        scroll->setWidget(tilePanel);layout->addWidget(scroll);
        resourceDock_->setWidget(content);
        consoleDock_=new QDockWidget("Consola",this);consoleDock_->setObjectName("DockConsola");
        console_=new QLabel("Motor MSX2 listo.");console_->setWordWrap(true);console_->setAlignment(Qt::AlignLeft|Qt::AlignTop);
        auto *container=new QWidget;auto *vl=new QVBoxLayout(container);vl->addWidget(console_);vl->addStretch();consoleDock_->setWidget(container);
    }
    void makeCentral() {
        centralTabs_=new QTabWidget;
        auto *scroll=new QScrollArea;scroll->setWidgetResizable(false);scroll->setAlignment(Qt::AlignCenter);
        map_=new MapCanvas;map_->project=project_.get();map_->clicked=[this](int x,int y,Qt::MouseButton button){pick(x,y,button);};
        map_->setZoom(3);scroll->setWidget(map_);
        centralTabs_->addTab(scroll,"Mapa");
        { // Editor de sprites Qt6 real, conectado a la misma estructura del motor.
            auto *page=new QWidget;auto *outer=new QVBoxLayout(page);
            auto *controls=new QHBoxLayout;
            auto *slot=new QComboBox;
            for(int i=0;i<NPC_ART_SLOTS;++i)slot->addItem(QString("Personaje %1").arg(i+1));
            controls->addWidget(new QLabel("Apariencia:"));controls->addWidget(slot);
            controls->addStretch();
            auto *clear=new QPushButton("Limpiar sprite");controls->addWidget(clear);
            outer->addLayout(controls);
            spriteCanvas_=new SpriteCanvas;spriteCanvas_->project=project_.get();
            outer->addWidget(spriteCanvas_,1);
            auto *colors=new QHBoxLayout;
            auto *paletteLabel=new QLabel("Color:");colors->addWidget(paletteLabel);
            for(int i=0;i<16;++i){
                auto *btn=new QPushButton(QString::number(i));
                btn->setToolTip(QString("Color MSX2 %1").arg(i));
                btn->setFixedWidth(35);
                btn->setStyleSheet(QString("background-color:%1;color:%2;")
                    .arg(QColor::fromRgb(forge_msx2_palette[i]).name(),i==1||i==0?"white":"black"));
                connect(btn,&QPushButton::clicked,this,[this,i]{spriteCanvas_->color=i;});
                colors->addWidget(btn);
            }
            outer->addLayout(colors);
            auto *hint=new QLabel("Clic izquierdo: dibujar · Clic derecho: borrar · Ctrl+Z: deshacer. Los cambios se guardan en .msxf.");
            hint->setObjectName("help");hint->setWordWrap(true);outer->addWidget(hint);
            connect(slot,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int i){
                spriteCanvas_->slot=qBound(0,i,NPC_ART_SLOTS-1);spriteCanvas_->update();
            });
            spriteCanvas_->beginStroke=[this]{snapshot();};
            spriteCanvas_->changed=[this]{changed();refreshArt();map_->update();};
            connect(clear,&QPushButton::clicked,this,[this]{
                if(QMessageBox::question(this,"Limpiar sprite","¿Borrar la apariencia seleccionada?")!=QMessageBox::Yes)return;
                snapshot();std::memset(project_->npc_sprites[spriteCanvas_->slot],255,256);
                spriteCanvas_->update();changed();refreshArt();map_->update();
            });
            centralTabs_->addTab(page,"Sprites");
        }
        for(const QString &name:{QStringLiteral("Eventos"),QStringLiteral("Música")}){
            auto *page=new QWidget;auto *v=new QVBoxLayout(page);
            auto *message=new QLabel(name+"\n\nPendiente de integración nativa. El motor y los proyectos existentes permanecen intactos.");
            message->setAlignment(Qt::AlignCenter);message->setWordWrap(true);v->addWidget(message);
            const int tab=centralTabs_->addTab(page,name+" (pendiente)");centralTabs_->setTabEnabled(tab,false);
        }
        setCentralWidget(centralTabs_);
    }
    void makeMenus() {
        auto *file=menuBar()->addMenu("Archivo");
        auto *actNew=file->addAction("Nuevo proyecto");actNew->setShortcut(QKeySequence::New);connect(actNew,&QAction::triggered,this,[this]{newProject();});
        auto *actOpen=file->addAction("Abrir…");actOpen->setShortcut(QKeySequence::Open);connect(actOpen,&QAction::triggered,this,[this]{loadProject();});
        auto *actSave=file->addAction("Guardar");actSave->setShortcut(QKeySequence::Save);connect(actSave,&QAction::triggered,this,[this]{save();});
        auto *actSaveAs=file->addAction("Guardar como…");actSaveAs->setShortcut(QKeySequence::SaveAs);connect(actSaveAs,&QAction::triggered,this,[this]{save(true);});
        file->addSeparator();auto *actRom=file->addAction("Exportar ROM…");actRom->setShortcut(QKeySequence("Ctrl+B"));connect(actRom,&QAction::triggered,this,[this]{exportRom(false);});
        file->addSeparator();auto *actDemo=file->addAction("Cargar demostración");connect(actDemo,&QAction::triggered,this,[this]{newProject(true);});
        auto *edit=menuBar()->addMenu("Editar");
        auto *actUndo=edit->addAction("Deshacer");actUndo->setShortcut(QKeySequence::Undo);connect(actUndo,&QAction::triggered,this,[this]{undo();});
        auto *actRedo=edit->addAction("Rehacer");actRedo->setShortcut(QKeySequence::Redo);connect(actRedo,&QAction::triggered,this,[this]{undo(true);});
        auto *view=menuBar()->addMenu("Ver");
        view->addAction("Restaurar paneles predeterminados",this,[this]{defaultDocking();});
        for(auto *dock:{explorerDock_,libraryDock_,inspectorDock_,resourceDock_,consoleDock_})view->addAction(dock->toggleViewAction());
        auto *tools=menuBar()->addMenu("Herramientas");
        auto *test=tools->addAction("Probar ROM en openMSX");test->setShortcut(QKeySequence("F5"));connect(test,&QAction::triggered,this,[this]{exportRom(true);});
        auto *bar=addToolBar("Herramientas de mapa");bar->setObjectName("MapTools");bar->setMovable(false);
        auto *brand=new QLabel;brand->setPixmap(QPixmap(":/msx_logo.png").scaledToHeight(28,Qt::SmoothTransformation));
        brand->setToolTip("MSX Forge Studio 0.6.0");bar->addWidget(brand);bar->addSeparator();
        auto addTool=[this,bar](const QString &name,Tool tool,const QString &shortcut){
            auto *a=bar->addAction(name);a->setCheckable(true);a->setShortcut(QKeySequence(shortcut));
            connect(a,&QAction::triggered,this,[this,tool,a,bar]{
                chooseTool(tool);
                for(QAction *other:bar->actions())if(other->isCheckable())other->setChecked(other==a);
            });return a;
        };
        addTool("Seleccionar",Tool::Select,"V")->setChecked(true);
        addTool("Pincel",Tool::Brush,"B");
        addTool("Colocar NPC",Tool::InsertNPC,"N");
        addTool("Colocar puerta",Tool::InsertDoor,"D");
        bar->addSeparator();bar->addWidget(new QLabel("  Destino: "));
        destPick_=new QComboBox;bar->addWidget(destPick_);
        bar->addSeparator();
        auto *zoomOut=bar->addAction("− Zoom");connect(zoomOut,&QAction::triggered,this,[this]{map_->setZoom(map_->zoom-1);});
        auto *zoomIn=bar->addAction("+ Zoom");connect(zoomIn,&QAction::triggered,this,[this]{map_->setZoom(map_->zoom+1);});
        bar->addSeparator();
        auto *saveTop=bar->addAction("Guardar");connect(saveTop,&QAction::triggered,this,[this]{save();});
        auto *testTop=bar->addAction("▶ Probar ROM");connect(testTop,&QAction::triggered,this,[this]{exportRom(true);});
        fileLabel_=new QLabel;statusBar()->addPermanentWidget(fileLabel_);
    }
protected:
    void closeEvent(QCloseEvent *event) override {
        if(choosingArrival_)cancelArrival();
        if(!guardChanges()){event->ignore();return;}
        QSettings settings("MSXForge","Studio060");settings.setValue("windowGeometry",saveGeometry());
        settings.setValue("windowState",saveState(1));event->accept();
    }
public:
    MainWindow() {
        forge_blank(project_.get());
        resize(1400,860);setMinimumSize(740,440);
        setDockOptions(QMainWindow::AnimatedDocks|QMainWindow::AllowNestedDocks|QMainWindow::AllowTabbedDocks|QMainWindow::GroupedDragging);
        setDockNestingEnabled(true);setTabPosition(Qt::AllDockWidgetAreas,QTabWidget::North);
        makeCentral();makeExplorer();makeLibrary();makeInspector();makeResources();makeMenus();
        setWindowIcon(QIcon(":/msx_logo.png"));
        defaultDocking();rebuildTree();updateInspector();title();
        QSettings settings("MSXForge","Studio060");
        if(settings.contains("windowGeometry"))restoreGeometry(settings.value("windowGeometry").toByteArray());
        if(settings.contains("windowState"))restoreState(settings.value("windowState").toByteArray(),1);
        QTimer::singleShot(0,this,[this]{
            recoverFloatingDocks();
            bool visible=false;
            for(QScreen *screen:QGuiApplication::screens())
                if(screen->availableGeometry().intersects(frameGeometry())){visible=true;break;}
            if(!visible&&QGuiApplication::primaryScreen()){
                const QRect available=QGuiApplication::primaryScreen()->availableGeometry();
                resize(qMin(width(),available.width()),qMin(height(),available.height()));
                move(available.topLeft());
            }
        });
    }
};

int main(int argc,char **argv) {
    QApplication app(argc,argv);
    QCoreApplication::setOrganizationName("MSXForge");
    QCoreApplication::setApplicationName("MSX Forge Studio 0.6.0");
    app.setStyleSheet(style());
    MainWindow window;window.show();
    return app.exec();
}
