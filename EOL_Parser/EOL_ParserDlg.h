#pragma once
#include <vector> // [추가] std::vector 사용을 위해 필요
// EOL_ParserDlg.h 에 추가
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
    // [추가] 로직 함수 선언 (cpp에 있는 함수들)
    void ShowDatabaseContents(CString folderPath);
    CString ExtractHtmlValue(const CString& source, CString startTag, CString endTag, int& searchPos);

    CFont m_font; // [추가] 폰트 객체 변수

    // [변수 및 이벤트]
    CListCtrl m_listData;
    afx_msg void OnBnClickedBtnFolderopen();
    afx_msg void OnEnChangeFolderpath();

    // [추가] 통계용 리스트 컨트롤 변수
    CListCtrl m_listStats;

    // [추가] 통계 계산 및 출력 함수 선언
    void UpdateStatistics();

    CComboBox m_comboModel; // 콤보박스 변수
    afx_msg void OnCbnSelchangeComboModel(); // 콤보박스 변경 이벤트 함수

    // 통계 및 수학 함수
    double GetMean(const std::vector<double>& data);
    double GetStdDev(const std::vector<double>& data, double mean);
    double NormalPDF(double x);

    // 그리기 함수
    void DrawNormalDistribution(CPaintDC& dc, CRect rect, std::vector<double>& data, COLORREF color);
    afx_msg void OnLvnItemchangedListStats(NMHDR* pNMHDR, LRESULT* pResult);
};
