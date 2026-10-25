from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'qt/main.cpp').read_text()
for fragment in ['class SpriteCanvas final','centralTabs_->addTab(page,"Sprites")','spriteCanvas_->changed=[this]','project_->npc_sprites[spriteCanvas_->slot]','snapshot();','QMessageBox::question(this,"Limpiar sprite"']:
    assert fragment in s, fragment
print('PASS: native sprite Qt6 source integration')
