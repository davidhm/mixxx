

#include "preferences/listenbrainzsettings.h"

ListenBrainzSettings ListenBrainzSettingsManager::s_latestSettings;

ListenBrainzSettingsManager::ListenBrainzSettingsManager(
                UserSettingsPointer pSettings,
                const ListenBrainzWidgets& widgets)
        :  m_widgets(widgets),
           m_pUserSettings(pSettings),
           m_CPSettingsChanged(kListenBrainzSettingsChanged) {
    s_latestSettings = getPersistedSettings(pSettings);
    setUpWidgets();
}

ListenBrainzSettings ListenBrainzSettingsManager::getPersistedSettings(UserSettingsPointer pSettings) {
    ListenBrainzSettings ret;
    ret.enabled = pSettings->getValue(kListenBrainzEnabled,defaultListenBrainzEnabled);
    ret.userToken = pSettings->getValue(kListenBrainzUserToken,QString());
    return ret;
}

void ListenBrainzSettingsManager::setUpWidgets() {
    m_widgets.m_pEnabled->setChecked(s_latestSettings.enabled);
    if (!s_latestSettings.userToken.isEmpty()) {
        m_widgets.m_pUserToken->setText(s_latestSettings.userToken);
    }
}

ListenBrainzSettings ListenBrainzSettingsManager::getLatestSettings() {
    return s_latestSettings;
}


void ListenBrainzSettingsManager::applySettings() {
    if (settingsDifferent() && settingsCorrect()) {
        updateLatestSettingsAndNotify();
        persistSettings();
    }
}

bool ListenBrainzSettingsManager::settingsDifferent() {
     return s_latestSettings.enabled != m_widgets.m_pEnabled->isChecked() ||
            s_latestSettings.userToken != m_widgets.m_pUserToken->text();
}

void ListenBrainzSettingsManager::updateLatestSettingsAndNotify() {
    s_latestSettings.enabled = m_widgets.m_pEnabled->isChecked();
    s_latestSettings.userToken = m_widgets.m_pUserToken->text();
    m_CPSettingsChanged.set(true);
}

void ListenBrainzSettingsManager::persistSettings() {
    m_pUserSettings->setValue(kListenBrainzEnabled,s_latestSettings.enabled);
    m_pUserSettings->setValue(kListenBrainzUserToken,s_latestSettings.userToken);
}

void ListenBrainzSettingsManager::cancelSettings() {
    setUpWidgets();
}

void ListenBrainzSettingsManager::setSettingsToDefault() {
    resetSettingsToDefault();
    setUpWidgets();
}

void ListenBrainzSettingsManager::resetSettingsToDefault() {
    s_latestSettings.enabled = defaultListenBrainzEnabled;
    s_latestSettings.userToken = QString();
}

bool ListenBrainzSettingsManager::settingsCorrect() {
    return !m_widgets.m_pEnabled->isChecked() ||
           !m_widgets.m_pUserToken->text().isEmpty();
}
