#pragma once

#include "broadcast/scrobblingservice.h"
#include "preferences/dlgpreferencepage.h"
#include "preferences/dialog/ui_dlgprefmetadatadlg.h"
#include "preferences/metadatafilesettings.h"
#include "preferences/listenbrainzsettings.h"
#include "preferences/usersettings.h"

namespace Ui {
    class fileListenerBox;
}

namespace {


    const ConfigKey kListenbrainzEnabled =
            ConfigKey("[Livemetadata]","ListenbrainzEnabled");


};



class DlgPrefMetadata : public DlgPreferencePage, public Ui::DlgPrefMetadataDlg {
  Q_OBJECT
  public:
    DlgPrefMetadata(QWidget *pParent, UserSettingsPointer pSettings);
    ~DlgPrefMetadata();
  public slots:
    void slotApply() override;
    void slotCancel() override;
    void slotResetToDefaults() override;
  private:
    UserSettingsPointer m_pSettings;
    MetadataFileSettings *m_pFileSettings;
    ListenBrainzSettingsManager *m_pListenBrainzSettings;
    void setFileSettings();

    void setListenBrainzSettings();
};



