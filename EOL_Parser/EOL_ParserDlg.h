#pragma once
#include <vector>
#include <algorithm>

// [추가] 데이터를 담을 구조체 선언
struct TestResult {
    CString serialNumber;
    std::vector<double> lpe;
    std::vector<double> lps;
};

// CEOLParserDlg 대화 상자
class CEOLParserDlg : public CDialogEx
{
public:
    CEOLParserDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_EOL_PARSER_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

protected:
    HICON m_hIcon;

    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

public:
    // [UI 컨트롤 변수]
    CListCtrl m_listData;    // 메인 데이터 리스트
    CListCtrl m_listStats;   // 통계 리스트
    CListBox  m_logList;     // 로그 리스트
    CComboBox m_comboModel;  // 모델 선택 콤보박스
    CFont     m_font;        // 폰트 객체
    CFont m_fontGroup; // 그룹박스용 폰트 변수
    CMFCButton mbtn_save;
    CMFCButton mbtn_quit;
    CMFCButton mbtn_reset;
    CMFCButton mbtn_folder;


    // [이벤트 핸들러]
    afx_msg void OnBnClickedBtnFolderopen();
    afx_msg void OnEnChangeFolderpath();
    afx_msg void OnCbnSelchangeComboModel();

    // (자동 생성된 이벤트 핸들러 - 사용 안 하면 지워도 됨)
    afx_msg void OnLvnItemchangedListStats(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLvnItemchangedListLog(NMHDR* pNMHDR, LRESULT* pResult);

    // [사용자 정의 함수]
    void ShowDatabaseContents(CString folderPath);
    CString ExtractHtmlValue(const CString& source, CString startTag, CString endTag, int& searchPos);
    void UpdateStatistics(); // 통계 계산
    void AddLog(CString msg); // 로그 출력

    // [통계 및 그래프 함수]
    double GetMean(const std::vector<double>& data);
    double GetStdDev(const std::vector<double>& data, double mean);
    double NormalPDF(double x);
    void DrawNormalDistribution(CPaintDC& dc, CRect rect, std::vector<double>& data, COLORREF color);

 

};