/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "config.h"

#include "katepluginmanager.h"

#include "kateapp.h"
#include "katemainwindow.h"

#include <KConfig>

#include <KServiceTypeTrader>
#include <KConfigGroup>
#include "katedebug.h"
#include <QFile>

#include <ktexteditor/sessionconfiginterface.h>

QString KatePluginInfo::saveName() const
{
    return service->library();
}

KatePluginManager::KatePluginManager(QObject *parent) : QObject(parent)
{
    setupPluginList();
}

KatePluginManager::~KatePluginManager()
{
    // than unload the plugins
    unloadAllPlugins();
}

void KatePluginManager::setupPluginList()
{
    KService::List traderList = KServiceTypeTrader::self()->query(QStringLiteral("KTextEditor/Plugin"));

    m_pluginList.clear ();
    foreach(const KService::Ptr & ptr, traderList) {
        KatePluginInfo info;
        info.service = ptr;
        info.defaultLoad = info.service->property(QStringLiteral("X-KTextEditor-Load-Default")).toStringList().contains(QStringLiteral("kate"));
        info.load = false;
        info.plugin = nullptr;
        m_pluginList.push_back(info);
    }

    /**
     * construct fast lookup map
     */
    m_name2Plugin.clear();
    for (int i = 0; i < m_pluginList.size(); ++i) {
        m_name2Plugin[m_pluginList[i].service->library()] = &(m_pluginList[i]);
    }
}

void KatePluginManager::loadConfig(KConfig *config)
{
    // first: unload the plugins
    unloadAllPlugins();

    /**
     * ask config object
     */
    if (config) {
        KConfigGroup cg = KConfigGroup(config, QStringLiteral("Kate Plugins"));

        // disable all plugin if no config, beside the ones marked as default load
        for (int i = 0; i < m_pluginList.size(); ++i) {
            m_pluginList[i].load = cg.readEntry(m_pluginList[i].service->library(), m_pluginList[i].defaultLoad);
        }
    }

    /**
     * load plugins
     */
    for (KatePluginList::iterator it = m_pluginList.begin(); it != m_pluginList.end(); ++it) {
        if (it->load) {
            loadPlugin(&(*it));

            // restore config
            if (auto interface = qobject_cast<KTextEditor::SessionConfigInterface *> (it->plugin)) {
                KConfigGroup group(config, QString::fromLatin1("Plugin:%1:").arg(it->saveName()));
                interface->readSessionConfig(group);
            }
        }
    }
}

void KatePluginManager::writeConfig(KConfig *config)
{
    Q_ASSERT(config);

    KConfigGroup cg = KConfigGroup(config, QStringLiteral("Kate Plugins"));
    foreach(const KatePluginInfo & plugin, m_pluginList) {
        QString saveName = plugin.saveName();

        cg.writeEntry(saveName, plugin.load);

        // save config
        if (auto interface = qobject_cast<KTextEditor::SessionConfigInterface *> (plugin.plugin)) {
            KConfigGroup group(config, QString::fromLatin1("Plugin:%1:").arg(saveName));
            interface->writeSessionConfig(group);
        }
    }
}

void KatePluginManager::unloadAllPlugins()
{
    for (KatePluginList::iterator it = m_pluginList.begin(); it != m_pluginList.end(); ++it) {
        if (it->plugin) {
            unloadPlugin(&(*it));
        }
    }
}

void KatePluginManager::enableAllPluginsGUI(KateMainWindow *win, KConfigBase *config)
{
    for (KatePluginList::iterator it = m_pluginList.begin(); it != m_pluginList.end(); ++it) {
        if (it->plugin) {
            enablePluginGUI(&(*it), win, config);
        }
    }
}

void KatePluginManager::disableAllPluginsGUI(KateMainWindow *win)
{
    for (KatePluginList::iterator it = m_pluginList.begin(); it != m_pluginList.end(); ++it) {
        if (it->plugin) {
            disablePluginGUI(&(*it), win);
        }
    }
}

void KatePluginManager::loadPlugin(KatePluginInfo *item)
{
    const QString pluginName = item->service->library();
    item->load = (item->plugin = item->service->createInstance<KTextEditor::Plugin>(this, QVariantList() << pluginName));
    if (item->plugin) {
        emit KateApp::self()->wrapper()->pluginCreated(pluginName, item->plugin);
    }
}

void KatePluginManager::unloadPlugin(KatePluginInfo *item)
{
    disablePluginGUI(item);
    delete item->plugin;
    KTextEditor::Plugin *plugin = item->plugin;
    item->plugin = 0L;
    item->load = false;
    emit KateApp::self()->wrapper()->pluginDeleted(item->service->library(), plugin);
}

void KatePluginManager::enablePluginGUI(KatePluginInfo *item, KateMainWindow *win, KConfigBase *config)
{
    // plugin around at all?
    if (!item->plugin) {
        return;
    }

    // lookup if there is already a view for it..
    QObject *createdView = nullptr;
    if (!win->pluginViews().contains(item->plugin)) {
        // create the view + try to correctly load shortcuts, if it's a GUI Client
        createdView = item->plugin->createView(win->wrapper());
        if (createdView) {
            win->pluginViews().insert(item->plugin, createdView);
        }
    }

    // load session config if needed
    if (config && win->pluginViews().contains(item->plugin)) {
        if (auto interface = qobject_cast<KTextEditor::SessionConfigInterface *> (win->pluginViews().value(item->plugin))) {
            KConfigGroup group(config, QString::fromLatin1("Plugin:%1:MainWindow:0").arg(item->saveName()));
            interface->readSessionConfig(group);
        }
    }

    if (createdView) {
        emit win->wrapper()->pluginViewCreated(item->service->library(), createdView);
    }
}

void KatePluginManager::enablePluginGUI(KatePluginInfo *item)
{
    // plugin around at all?
    if (!item->plugin) {
        return;
    }

    // enable the gui for all mainwindows...
    for (int i = 0; i < KateApp::self()->mainWindowsCount(); i++) {
        enablePluginGUI(item, KateApp::self()->mainWindow(i), 0);
    }
}

void KatePluginManager::disablePluginGUI(KatePluginInfo *item, KateMainWindow *win)
{
    // plugin around at all?
    if (!item->plugin) {
        return;
    }

    // lookup if there is a view for it..
    if (!win->pluginViews().contains(item->plugin)) {
        return;
    }

    // really delete the view of this plugin
    QObject *deletedView = win->pluginViews().value(item->plugin);
    delete deletedView;
    win->pluginViews().remove(item->plugin);
    emit win->wrapper()->pluginViewDeleted(item->service->library(), deletedView);
}

void KatePluginManager::disablePluginGUI(KatePluginInfo *item)
{
    // plugin around at all?
    if (!item->plugin) {
        return;
    }

    // disable the gui for all mainwindows...
    for (int i = 0; i < KateApp::self()->mainWindowsCount(); i++) {
        disablePluginGUI(item, KateApp::self()->mainWindow(i));
    }
}

KTextEditor::Plugin *KatePluginManager::plugin(const QString &name)
{
    /**
     * name known?
     */
    if (!m_name2Plugin.contains(name)) {
        return 0;
    }

    /**
     * real plugin instance, if any ;)
     */
    return m_name2Plugin.value(name)->plugin;
}

bool KatePluginManager::pluginAvailable(const QString &name)
{
    return m_name2Plugin.contains(name);
}

class KTextEditor::Plugin *KatePluginManager::loadPlugin(const QString &name, bool permanent)
{
    /**
     * name known?
     */
    if (!m_name2Plugin.contains(name)) {
        return 0;
    }

    /**
     * load, bail out on error
     */
    loadPlugin(m_name2Plugin.value(name));
    if (!m_name2Plugin.value(name)->plugin) {
        return 0;
    }

    /**
     * perhaps patch not load again back to "ok, load it once again on next loadConfig"
     */
    m_name2Plugin.value(name)->load = permanent;
    return m_name2Plugin.value(name)->plugin;
}

void KatePluginManager::unloadPlugin(const QString &name, bool permanent)
{
    /**
     * name known?
     */
    if (!m_name2Plugin.contains(name)) {
        return;
    }

    /**
     * unload
     */
    unloadPlugin(m_name2Plugin.value(name));

    /**
     * perhaps patch load again back to "ok, load it once again on next loadConfig"
     */
    m_name2Plugin.value(name)->load = !permanent;
}

