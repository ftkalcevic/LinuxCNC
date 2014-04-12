#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "gwizard.h"
#include "mru.h"
#include "gcodepreviewglwidget.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(GWizard &gwiz, QWidget *parent = 0);
    ~MainWindow();
    
    void ShowWizFile();
    void EditCurrentText(bool bEditing);
    void CreateNewWizard();
    void DeleteCurrentListItem();
    void CutCurrentListItem();
    void CopyCurrentListItem();
    void PasteCurrentListItem();
    void CreatePreviewPanelContents();
private:
    Ui::MainWindow *ui;
    GWizard &m_gwiz;
    GCodePreviewGLWidget *m_glPreview;
    QComboBox *m_cboViews;
    MRU m_MRU;
    QScopedPointer<WizLine> m_WizLineClipboard;
    bool m_bEditing;
    
    void BuildTree( QTreeWidgetItem *parent, Wizard *wz );
    void SaveWindowLayout();
    void RestoreWindowLayout();
    
public slots:
    void OnTreeCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void OnActionOpen();
    void OnListCurrentItemChanged(const QModelIndex &,const QModelIndex &);
    void onMRUSelected(const QString &);
    void onParamPanelWidgetChanged(const Parameter &param);
    void OnActionEditWizard();
    void OnActionAccept();
    void OnActionReject();
    void OnActionInsertWizard();
    void OnActionInsertBlank();
    void OnEditLineTextEdited(const QString &);
    void OnEditLineReturnPressed();
    void OnEditLineEscapePressed();
    void OnActionCut();
    void OnActionDown();
    void OnActionUp();
    void OnActionSaveAs();
    void OnActionSave();
    void OnActionNew();
    void OnActionPaste();
    void OnActionCopy();
    void OnActionDelete();
    void OnListClicked(const QModelIndex &);
    void OnListDoubleClicked(const QModelIndex &current);
    void OnTreeDoubleClicked(const QModelIndex &);
    void OnListviewDeletePressed();
    void OnListviewCutPressed();
    void OnListviewCopyPressed();
    void OnListviewPastePressed();
    void OnListviewUpPressed();
    void OnListviewDownPressed();
    void OnActionViewChanged(int row);
    void OnActionGrid(bool bGridOn);
protected:
    virtual void closeEvent(QCloseEvent *);
private:
    void BuildParamPanel(const Wizard * const wz, QStringList *lstParamValues);
    void SelectWizard(const Wizard * const wz, QStringList *params );
    void LoadWizFile(const QString sFilename);
    void ClearParamPanel();
    void WizardSelected(const Wizard * const wz);
    void UpdateButtonState();
    void LoadPreview(bool bResetView = false);
    void EditCurrentWizard(bool bEditing);
    void ClearDirty();
    void SetDirty();
    void LineEditingDone(bool bAccept);
    bool DoSave();
    bool DoSaveAs();
    int GetListNewRow() const;
    bool SaveChanges();
    QStringList GetPanelParameters(Wizard *wiz);
};

#endif // MAINWINDOW_H
