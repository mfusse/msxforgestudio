#pragma once
#include <QByteArray>
#include <QString>
extern "C" {
#include "engine.h"
}

namespace ProjectIO {
// Size-and-version-aware import of legacy and current binary .msxf projects.
// Returns false without mutating `target` when a file is invalid.
bool fromBytes(const QByteArray &bytes, ForgeProject &target, QString &error);
bool load(const QString &path, ForgeProject &target, QString &error);
bool save(const QString &path, const ForgeProject &project, QString &error);
bool exportRom(const QString &path, const ForgeProject &project, QString &error);
}
