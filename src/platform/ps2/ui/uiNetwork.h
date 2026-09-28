#ifndef _UINETWORK_H
#define _UINETWORK_H

#include "uiScreen.h"
#include "mainloop_smb.h"

class CNetworkScreen : public CScreen
{
    int m_iSelect;
    int m_iDigitIP;
    /* AURORA_SMB_NET_FROM_CNF_20260924: which m_Config IP field the digit
       editor is bound to (0=serverIp, 1=ps2Ip, 2=netmask, 3=gateway). */
    int m_iIpTarget;
    int m_iEditField;
    int m_iTextCursor;
    Int8 m_NetworkIP[12];
    Bool m_bLoaded;
    SmbConfigT m_Config;
    /* AURORA_SMB_OSK_V1_20260924: on-screen keyboard state for the text
       fields (row/col of the highlighted key, upper-case toggle, and the
       pre-edit backup restored by the cancel button). */
    int m_iOskRow;
    int m_iOskCol;
    Bool m_bOskUpper;
    char m_OskBackup[256];

    void LoadConfig();
    void SetEditIP(const char *address);
    char *GetIpTarget(int field);
    const char *GetIpDisplay(int field) const;
    void BeginIpEdit(int field);
    void CommitEditIP();
    int GetOctet(int index) const;
    char *GetEditText(int field, int *maxLength);
    void BeginTextEdit(int field);
    void InputIP(Uint32 trigger);
    void InputText(Uint32 trigger);
    void DrawIP(int x, int y);
    void DrawIpRow(int y, int index, const char *label, int field);
    void DrawOsk();
    void BuildDisplayText(char *output, int outputSize, const char *text,
                          int password, int editing);

public:
    CNetworkScreen();

    void Draw();
    void Process();
    void Input(Uint32 buttons, Uint32 trigger);
};

#endif
