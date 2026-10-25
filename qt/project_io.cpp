#include "project_io.h"
#include <QFile>
#include <QSaveFile>
#include <cstring>
#include <memory>

namespace ProjectIO {
bool fromBytes(const QByteArray &bytes, ForgeProject &target, QString &error) {
    if (bytes.isEmpty() || bytes.size() > MAX_PROJECT_BYTES) {
        error = QStringLiteral("Archivo vacío o demasiado grande para el formato MSX Forge.");
        return false;
    }
    auto temp = std::make_unique<ForgeProject>();
    bool ok = false;
    if (bytes.size() == static_cast<qsizetype>(sizeof(ForgeProject))) {
        std::memcpy(temp.get(), bytes.constData(), sizeof(ForgeProject));
        ok = temp->magic == KEY_MAGIC && temp->version == FORGE_VERSION;
    } else if (bytes.size() == static_cast<qsizetype>(sizeof(ForgeProjectV11))) {
        auto prior = std::make_unique<ForgeProjectV11>();
        std::memcpy(prior.get(), bytes.constData(), sizeof(ForgeProjectV11));
        ok = forge_import_v11(temp.get(), prior.get(), unsigned(bytes.size())) != 0;
    } else if (bytes.size() >= FORGE_V1_FILE_BYTES && bytes.size() <= static_cast<qsizetype>(sizeof(ForgeProjectV10))) {
        auto prior = std::make_unique<ForgeProjectV10>();
        std::memset(prior.get(), 0, sizeof(ForgeProjectV10));
        std::memcpy(prior.get(), bytes.constData(), size_t(bytes.size()));
        ok = forge_import_legacy(temp.get(), prior.get(), unsigned(bytes.size())) != 0;
    }
    char reason[256]{};
    if (ok) ok = forge_validate(temp.get(), reason, sizeof(reason)) != 0;
    if (!ok) {
        error = reason[0] ? QString::fromUtf8(reason) : QStringLiteral("Formato .msxf incompatible o datos inválidos.");
        return false;
    }
    target = *temp;
    error.clear();
    return true;
}

bool load(const QString &path, ForgeProject &target, QString &error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }
    if (file.size() > MAX_PROJECT_BYTES) {
        error = QStringLiteral("Proyecto demasiado grande.");
        return false;
    }
    return fromBytes(file.readAll(), target, error);
}

bool save(const QString &path, const ForgeProject &project, QString &error) {
    ForgeProject copy = project;
    copy.magic = KEY_MAGIC;
    copy.version = FORGE_VERSION;
    char reason[256]{};
    if (!forge_validate(&copy, reason, sizeof(reason))) {
        error = QString::fromUtf8(reason);
        return false;
    }
    QSaveFile output(path); // Atomic replacement: no half-written project if disk errors occur.
    if (!output.open(QIODevice::WriteOnly)) {
        error = output.errorString();
        return false;
    }
    if (output.write(reinterpret_cast<const char *>(&copy), sizeof(copy)) != qint64(sizeof(copy)) || !output.commit()) {
        error = output.errorString();
        return false;
    }
    error.clear();
    return true;
}

bool exportRom(const QString &path, const ForgeProject &project, QString &error) {
    QByteArray buffer(32768, '\0');
    char reason[256]{};
    const int count = forge_export_rom(&project, reinterpret_cast<uint8_t *>(buffer.data()), buffer.size(), reason, sizeof(reason));
    if (count != 32768) {
        error = QString::fromUtf8(reason);
        return false;
    }
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly)) {
        error = output.errorString();
        return false;
    }
    if (output.write(buffer.constData(), count) != count || !output.commit()) {
        error = output.errorString();
        return false;
    }
    error.clear();
    return true;
}
}
