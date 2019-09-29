#include "stdafx.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "wizlineoword.h"
#include "wizlinetext.h"


MainWindow::MainWindow(GWizard &gwiz, QWidget *parent) 
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_gwiz( gwiz )
    , m_glPreview( NULL )
    , m_cboViews( NULL )
    , m_bEditing( false )
{
    ui->setupUi(this);
    
    m_MRU.setMenu( ui->menuFile );
    
    // Add actions to push buttons - don't seem to be able to do this in the designer
    ui->btnDwn->setDefaultAction( ui->actionDown );
    ui->btnUp->setDefaultAction( ui->actionUp );
    ui->btnInsertBlank->setDefaultAction(ui->actionInsertBlank);
    ui->btnInsertWizard->setDefaultAction(ui->actionInsertWizard);
    
    ui->btnAccept->setDefaultAction(ui->actionAccept);
    ui->btnReject->setDefaultAction(ui->actionReject);
    ui->btnEditWizard->setDefaultAction(ui->actionEditWizard);
    ui->btnSetWizard->setDefaultAction(ui->actionSetWizard);

    // Tweak background of scroll area otherwise it stays a darker grey
    QPalette p( ui->scrollAreaParams->palette() );
    p.setColor( QPalette::Window, Qt::transparent);
    ui->scrollAreaParams->setPalette( p );
            
    RestoreWindowLayout();
    
    // Build the tree
    foreach ( Wizard *wz, m_gwiz.Wizards() )
    {
        BuildTree( NULL, wz );
    }
    
    connect( ui->actionOpen, SIGNAL(triggered()), this, SLOT(OnActionOpen()) );
    connect( ui->actionSave, SIGNAL(triggered()), this, SLOT(OnActionSave()) );
    connect( ui->actionSave_As, SIGNAL(triggered()), this, SLOT(OnActionSaveAs()) );
    connect( ui->actionNew, SIGNAL(triggered()), this, SLOT(OnActionNew()) );
    connect( ui->actionExit, SIGNAL(triggered()), this, SLOT(close()) );
    
    connect( ui->actionEditWizard, SIGNAL(triggered()), this, SLOT(OnActionEditWizard()) );
    connect( ui->actionInsertBlank, SIGNAL(triggered()), this, SLOT(OnActionInsertBlank()) );
    connect( ui->actionInsertWizard, SIGNAL(triggered()), this, SLOT(OnActionInsertWizard()) );
    //connect( ui->actionSetWizard, SIGNAL(triggered()), this, SLOT(OnActionSetWizard()) );
    
    connect( ui->actionDown, SIGNAL(triggered()), this, SLOT(OnActionDown()));
    connect( ui->actionUp, SIGNAL(triggered()), this, SLOT(OnActionUp()));
    connect( ui->actionCut, SIGNAL(triggered()), this, SLOT(OnActionCut()) );
    connect( ui->actionCopy, SIGNAL(triggered()), this, SLOT(OnActionCopy()) );
    connect( ui->actionPaste, SIGNAL(triggered()), this, SLOT(OnActionPaste()) );
    connect( ui->actionAccept, SIGNAL(triggered()), this, SLOT(OnActionAccept()) );
    connect( ui->actionReject, SIGNAL(triggered()), this, SLOT(OnActionReject()) );
    connect( ui->actionDelete, SIGNAL(triggered()), this, SLOT(OnActionDelete()) );
    connect( ui->actionGrid, SIGNAL(triggered(bool)), this, SLOT(OnActionGrid(bool)));
    
    connect( ui->lstWizFile, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(OnListDoubleClicked(const QModelIndex &)));
    connect( ui->lstWizFile, SIGNAL(clicked(const QModelIndex &)), this, SLOT(OnListClicked(const QModelIndex &)));
    connect( ui->lstWizFile, SIGNAL(deletePressed()), this, SLOT(OnListviewDeletePressed()) );
    connect( ui->lstWizFile, SIGNAL(cutPressed()), this, SLOT(OnListviewCutPressed()) );
    connect( ui->lstWizFile, SIGNAL(copyPressed()), this, SLOT(OnListviewCopyPressed()) );
    connect( ui->lstWizFile, SIGNAL(pastePressed()), this, SLOT(OnListviewPastePressed()) );
    connect( ui->lstWizFile, SIGNAL(moveUpPressed()), this, SLOT(OnListviewUpPressed()) );
    connect( ui->lstWizFile, SIGNAL(moveDownPressed()), this, SLOT(OnListviewDownPressed()) );
    connect( ui->editLine, SIGNAL(textEdited(const QString &)), this, SLOT(OnEditLineTextEdited(QString)) );
    connect( ui->editLine, SIGNAL(returnPressed()), this, SLOT(OnEditLineReturnPressed()));
    connect( ui->editLine, SIGNAL(escapePressed()), this, SLOT(OnEditLineEscapePressed()));
    connect( ui->treeWizards, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(OnTreeCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)) );
    connect( ui->treeWizards, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(OnTreeDoubleClicked(const QModelIndex &)) );
    connect( &m_MRU, SIGNAL(MRUSelected(const QString &)), this, SLOT(onMRUSelected(const QString &)) );
         
    CreatePreviewPanelContents();
    
    
    ui->treeWizards->setCurrentItem( ui->treeWizards->topLevelItem(0));
    
    ShowWizFile();
    UpdateButtonState();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::RestoreWindowLayout()
{
    QSettings settings;
    settings.beginGroup( "Layout");
        settings.beginGroup( "MainWindow");
            if ( settings.contains("pos") && settings.contains("size") && settings.contains("state") )
            {
                restoreState( settings.value("state").toByteArray() );
                resize( settings.value("size").toSize() );
                move( settings.value("pos").toPoint() );
            }
            else
            {
                // No initial layout.  Manually stack wizards and preview.
                this->tabifyDockWidget( ui->dockPreview, ui->dockWizards );
            }
        settings.endGroup();
        settings.beginGroup( "VerticalSplitter");
            if ( settings.contains("pos") && settings.contains("size") && settings.contains("state") )
            {
                ui->splitterVertical->restoreState( settings.value("state").toByteArray() );
                ui->splitterVertical->resize( settings.value("size").toSize() );
                ui->splitterVertical->move( settings.value("pos").toPoint() );
            }
        settings.endGroup();
    settings.endGroup();
    
    settings.beginGroup( "application");
        for ( int i = MAX_MRU-1; i >= 0; i-- )
        {
            QString sMRU = settings.value(QString("mru%1").arg(i), "" ).toString();
            if ( sMRU.length() > 0 )
                m_MRU.append( sMRU );
        }
    settings.endGroup();
    
}

void MainWindow::SaveWindowLayout()
{
    QSettings settings;
    settings.beginGroup( "Layout");
        settings.beginGroup( "MainWindow");
            settings.setValue("pos", pos());
            settings.setValue("size", size());
            settings.setValue("state", saveState());
        settings.endGroup();
        settings.beginGroup( "VerticalSplitter");
            settings.setValue("pos", ui->splitterVertical->pos());
            settings.setValue("size", ui->splitterVertical->size());
            settings.setValue("state", ui->splitterVertical->saveState());
        settings.endGroup();
    settings.endGroup();
    
    settings.beginGroup( "application");
        for ( int i = 0; i < MAX_MRU; i++ )
            if ( i < m_MRU.count() )
                settings.setValue( QString("mru%1").arg(i), m_MRU[i] );
        else
            settings.setValue( QString("mru%1").arg(i), "" );
    settings.endGroup();
   
}

void MainWindow::closeEvent(QCloseEvent *event )
{
    if ( !SaveChanges() )
    {
        event->ignore();
        return;
    }
    
    SaveWindowLayout();
}

void MainWindow::CreatePreviewPanelContents()
{
    m_glPreview = new GCodePreviewGLWidget(m_gwiz.isLathe(), this);
    
    //ui->dockPreviewContents->setLayout( NULL );
    //ui->dockPreviewContents->setLayout( new QVBoxLayout(this) );
    // Create toolbar
    QToolBar *tb = new QToolBar(this);
    tb->addAction( ui->actionGrid );
    if ( !m_gwiz.isLathe() )
    {
        tb->addWidget( new QLabel("View:",this) );
        m_cboViews = new QComboBox(tb);
        m_cboViews->insertItem(0, "X", ViewType::X );
        m_cboViews->insertItem(1, "Y", ViewType::Y );
        m_cboViews->insertItem(2, "Z", ViewType::Z );
        m_cboViews->insertItem(3, "Z2",ViewType::Z2 );
        m_cboViews->insertItem(4, "P", ViewType::P );
        m_cboViews->setCurrentIndex( m_cboViews->findData( m_glPreview->GetView() ));
        tb->addWidget( m_cboViews );
        connect( m_cboViews, SIGNAL(currentIndexChanged(int)), this, SLOT(OnActionViewChanged(int)));
    }
    ui->dockPreviewContents->layout()->addWidget( tb );
    ui->dockPreviewContents->layout()->addWidget( m_glPreview );
    m_glPreview->Init();    
}


void MainWindow::BuildTree( QTreeWidgetItem *parent, Wizard *wz )
{
    QTreeWidgetItem *child;
    if ( parent == NULL )
    {
        child = new QTreeWidgetItem( ui->treeWizards, QStringList(wz->GetTreeDesc()));        
        ui->treeWizards->insertTopLevelItem( 0, child );        
    }
    else
    {
        child = new QTreeWidgetItem( parent, QStringList(wz->GetTreeDesc()));        
    }
    child->setData(0,Qt::UserRole, qVariantFromValue((void *)wz));
    child->setExpanded(true);

    foreach ( Wizard *wzChild, wz->GetChildren() )
    {
        BuildTree( child, wzChild );
    }
}

void MainWindow::WizardSelected( const Wizard * const wz )
{
    if ( !wz )
    {
        ClearParamPanel();
    }
    else
    {
        BuildParamPanel( wz, NULL );
    }
}


void MainWindow::SelectWizard( const Wizard * const wz, QStringList *params )
{
    if ( wz != NULL )
    {
        QTreeWidgetItemIterator it(ui->treeWizards);
        while ( *it )
        {
            QTreeWidgetItem *w = *it;
            Wizard *twz = (Wizard *)w->data(0,Qt::UserRole).value<void *>();
            if ( twz == wz )
            {
                ui->treeWizards->setCurrentItem( w );
                BuildParamPanel( wz, params );
                return;
            }
            it++;
        }
    }
    ui->treeWizards->setCurrentItem(NULL);
    ClearParamPanel();
}

void MainWindow::OnTreeDoubleClicked(const QModelIndex &)
{
    CreateNewWizard();
}

void MainWindow::OnTreeCurrentItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * /*previous*/ )
{
    if ( !current )
    {
        WizardSelected( NULL );
    }
    else
    {
        const Wizard * const wz = (const Wizard * const )current->data(0,Qt::UserRole).value<void *>();    
        WizardSelected( wz );
    }
    UpdateButtonState();
}

void MainWindow::DeleteCurrentListItem()
{
    QModelIndex idx = ui->lstWizFile->currentIndex();
    if ( idx.isValid() )
    {
        m_gwiz.GetWizFile()->Cut( idx.row() );
        UpdateButtonState();
        LoadPreview();
    }    
}


void MainWindow::OnActionViewChanged(int row)
{
    if ( m_glPreview )
        m_glPreview->SetView( (ViewType::ViewTypes)m_cboViews->itemData(row,Qt::UserRole).toInt() );
}

void MainWindow::OnActionGrid(bool bGridOn)
{
    m_glPreview->SetGrid( bGridOn );
}

void MainWindow::OnActionDelete()
{
    DeleteCurrentListItem();
}

void MainWindow::OnListviewDeletePressed()
{
    if ( ui->actionDelete->isEnabled() )
        ui->actionDelete->trigger();
}

void MainWindow::OnListviewCutPressed()
{
    if ( ui->actionCut->isEnabled() )
        ui->actionCut->trigger();
}

void MainWindow::OnListviewCopyPressed()
{
    if ( ui->actionCopy->isEnabled() )
        ui->actionCopy->trigger();
}

void MainWindow::OnListviewPastePressed()
{
    if ( ui->actionPaste->isEnabled() )
        ui->actionPaste->trigger();
}

void MainWindow::OnListviewUpPressed()
{
    if ( ui->actionUp->isEnabled() )
        ui->actionUp->trigger();
}

void MainWindow::OnListviewDownPressed()
{
    if ( ui->actionDown->isEnabled() )
        ui->actionDown->trigger();
}


void MainWindow::OnListDoubleClicked(const QModelIndex &current)
{
    qDebug() << "DoubleClick";
    if ( current.isValid() )
    {
        if ( current.data(WizardRole).isValid() ) 
            EditCurrentWizard(true);
        else
            EditCurrentText(true);
    }
}

void MainWindow::OnListClicked(const QModelIndex &current)
{
    qDebug() << "Click";
    OnListCurrentItemChanged(current, QModelIndex() );
}

void MainWindow::OnListCurrentItemChanged(const QModelIndex &current,const QModelIndex &/*previous*/)
{
    if ( !current.isValid() )
        return;
    
    bool bDone = false;
    
    // Populate params, if this is a wizard line
    WizLine * wl = static_cast<WizLine *>(current.data(WizLineRole).value<void *>());
    WizLineOWord *wow = dynamic_cast<WizLineOWord *>(wl);

    if ( wow && m_gwiz.GetOWordMap().contains( wow->GetOWord() ) )
    {
        // Select the wizard, and populate the parameters
        ui->editLine->clear();
        SelectWizard( wow->GetWizard(), &(wow->GetParams()) );
        bDone = true;
    }
    
    if ( !bDone )
    {
        SelectWizard( NULL, NULL );
        ui->editLine->setText( wl->GetLine() );
    }
    
    int idx = current.row();
    if ( idx >= 0 )
        idx++;
    m_glPreview->HighlightLine( idx );
    UpdateButtonState();
}

void MainWindow::ClearParamPanel()
{
    // Clear out panel
    if ( ui->frameParams->layout() != NULL )
    {
        delete ui->frameParams->layout();
    }
    foreach(QObject * o, ui->frameParams->children() )
    {
        if ( o->isWidgetType() )
            delete o;
    }
    // And graphic
    ui->lblGraphic->clear();
    ui->lblDescription->clear();
}


void MainWindow::BuildParamPanel( const Wizard * const wz, QStringList *lstParamValues )
{
    // Clear out panel
    ClearParamPanel();

    // Create new panel    
    if ( wz->GetParameters().count() > 0 )
    {
        QFormLayout *layout = new QFormLayout( ui->frameParams );
        layout->setSpacing( 3 );
        layout->setMargin(0);
        layout->setLabelAlignment( Qt::AlignRight );
        ui->frameParams->setLayout( layout );

        int idx = 0;
        foreach ( const Parameter * const p, wz->GetParameters() )
        {
            QString sDefault = "";
            if ( lstParamValues != NULL )
            {
                sDefault = lstParamValues->at(idx);
            }
            
            QWidget *w = p->MakeWidget( ui->frameParams, sDefault );
            connect( p, SIGNAL(WidgetChanged(const Parameter &)), this, SLOT(onParamPanelWidgetChanged(const Parameter &)) );
            layout->addRow( p->GetDesc(), w );
            idx++;
        }
    }
    // And graphic
    ui->lblGraphic->setPixmap( wz->GetImage() );
    ui->lblDescription->setText( wz->GetParamDesc() );
}


void MainWindow::LineEditingDone( bool bAccept )
{
    if ( bAccept )
    {
        QModelIndex current = ui->lstWizFile->currentIndex();
        QString sNew = ui->editLine->text();
        if ( m_bEditing )
        {
            if ( !current.isValid() )
            {
                Q_ASSERT(false);
                return; 
            }
            
            WizLine * wl = static_cast<WizLine *>(current.data(WizLineRole).value<void *>());
            WizLineText *wltxt = dynamic_cast<WizLineText *>(wl);
            Q_ASSERT( wltxt );
        
            if ( sNew != wltxt->GetLine() )
            {
                wltxt->SetLine( sNew );
                m_gwiz.GetWizFile()->SetDirty( current.row() );
            }
        }
        else
        {
            int row = 0;
            if ( current.isValid() )
                row = current.row()+1;
            m_gwiz.GetWizFile()->AddTextRow( row, sNew );
            ui->lstWizFile->setCurrentIndex( ui->lstWizFile->model()->index( row, 0 ) );
        }
    }
    
    ui->lstWizFile->setEnabled(true);
    ui->frameParams->setEnabled(true);
    UpdateButtonState();
    LoadPreview();
}


void MainWindow::OnEditLineEscapePressed()
{
    LineEditingDone( false );
    ui->editLine->clear();
}

void MainWindow::OnEditLineReturnPressed()
{
    LineEditingDone( true );
    ui->editLine->clear();
}

void MainWindow::EditCurrentText( bool bEditing )
{
    // Can't move off list while editing
    if ( ui->lstWizFile->isEnabled() )
    {
        m_bEditing = bEditing;
        ui->lstWizFile->setEnabled(false);
        ui->frameParams->setEnabled(false);
        UpdateButtonState();
    }    
}

void MainWindow::OnEditLineTextEdited(const QString &)
{
    EditCurrentText(false);
}

void MainWindow::onParamPanelWidgetChanged(const Parameter & /*param*/)
{
    EditCurrentWizard(false);
}

void MainWindow::OnActionOpen()
{
    if ( SaveChanges() )
    {
        QFileDialog dlg(this, "Open wizard file", m_gwiz.GetWizFileDirectory(), "Wizard Files (*.wiz);;All Files (*)");
        dlg.setFileMode( QFileDialog::ExistingFile );
        dlg.setAcceptMode( QFileDialog::AcceptOpen );
        
        if ( dlg.exec() )
        {
            QString sFilename = dlg.selectedFiles()[0];
            LoadWizFile( sFilename );
        }
    }
}

// Returns true if we can continue
bool MainWindow::SaveChanges()
{
    if ( m_gwiz.GetWizFile()->isDirty() )
    {
        QMessageBox::StandardButton ret = QMessageBox::question( this, "Save changes?", "Save changes before closing?", QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard );
        if ( ret == QMessageBox::Cancel )
        {
            return false;
        }
        else if ( ret == QMessageBox::Save )
        {
            if ( !DoSave() )
            {
                return false;
            }
        }
    }
    return true;
}

void MainWindow::OnActionNew()
{
    if ( SaveChanges() )
    {
        m_gwiz.NewWizFile();
        ShowWizFile();
    }
}

bool MainWindow::DoSave()
{
    if ( m_gwiz.GetWizFile()->GetFilename().isEmpty() )
    {
        return DoSaveAs();
    }
    else
    {
        return m_gwiz.GetWizFile()->Save();
    }
}

bool MainWindow::DoSaveAs()
{
    QFileDialog dlg(this, "Save wizard file", m_gwiz.GetWizFileDirectory(), "Wizard Files (*.wiz);;All Files (*)");
    dlg.setFileMode( QFileDialog::AnyFile );
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setConfirmOverwrite( true );
    dlg.setDefaultSuffix( "wiz" );
    
    if ( dlg.exec() )
    {
        QString sFilename = dlg.selectedFiles()[0];
        if ( m_gwiz.GetWizFile()->SaveAs( sFilename ) )
        {
            m_MRU.append( sFilename );
            return true;
        }
    }
    return false;
}

void MainWindow::OnActionSave()
{
    DoSave();
}


void MainWindow::OnActionSaveAs()
{
    DoSaveAs();
}

void MainWindow::OnActionEditWizard()
{
    EditCurrentWizard(true);
}

int MainWindow::GetListNewRow() const
{
    if ( ui->lstWizFile->currentIndex().isValid() )
        return ui->lstWizFile->currentIndex().row();
    return 0;
}

void MainWindow::OnActionInsertBlank()
{
    int row = GetListNewRow();
    m_gwiz.GetWizFile()->AddTextRow( row, "" );
    ui->lstWizFile->setCurrentIndex( ui->lstWizFile->model()->index(row,0));
    LoadPreview();
}

void MainWindow::CreateNewWizard()
{
    Wizard *wiz = (Wizard *)ui->treeWizards->currentItem()->data(0,Qt::UserRole).value<void *>();
    int row = GetListNewRow();
    m_gwiz.GetWizFile()->AddWizardRow( row, wiz );
    ui->lstWizFile->setCurrentIndex( ui->lstWizFile->model()->index(row,0));
    LoadPreview();    
}

void MainWindow::OnActionInsertWizard()
{
    CreateNewWizard();
}

void MainWindow::OnActionDown()
{
    QModelIndex idx = ui->lstWizFile->currentIndex();
    if ( idx.isValid() )
    {
        m_gwiz.GetWizFile()->MoveDown( idx.row() );
        ui->lstWizFile->scrollTo( ui->lstWizFile->currentIndex());
        UpdateButtonState();
        LoadPreview();
    }
}

void MainWindow::OnActionUp()
{
    QModelIndex idx = ui->lstWizFile->currentIndex();
    if ( idx.isValid() )
    {
        m_gwiz.GetWizFile()->MoveUp( idx.row() );
        ui->lstWizFile->scrollTo( ui->lstWizFile->currentIndex());
        UpdateButtonState();
        LoadPreview();
    }
}

void MainWindow::CutCurrentListItem()
{
    QModelIndex idx = ui->lstWizFile->currentIndex();
    if ( idx.isValid() )
    {
        m_WizLineClipboard.reset( m_gwiz.GetWizFile()->Cut( idx.row() ) );
        UpdateButtonState();
        LoadPreview();
    }    
}

void MainWindow::OnActionCut()
{
    CutCurrentListItem();
}

void MainWindow::CopyCurrentListItem()
{
    QModelIndex idx = ui->lstWizFile->currentIndex();
    if ( idx.isValid() )
    {
        m_WizLineClipboard.reset( m_gwiz.GetWizFile()->GetWizLines()[idx.row()]->Clone() );
        UpdateButtonState();
    }    
}

void MainWindow::OnActionCopy()
{
    CopyCurrentListItem();
}

void MainWindow::PasteCurrentListItem()
{
    if ( !m_WizLineClipboard.isNull() )
    {
        int row = GetListNewRow();
        m_gwiz.GetWizFile()->AddLine( row, m_WizLineClipboard->Clone() );
        LoadPreview();
    }    
}

void MainWindow::OnActionPaste()
{
    PasteCurrentListItem();
}

QStringList MainWindow::GetPanelParameters( Wizard *wiz )
{
    QStringList sParams;
    if ( wiz->GetParameters().count() > 0 )
    {
        int idx = 0;
        foreach ( QObject *o, ui->frameParams->children() )
        {
            QWidget *w = dynamic_cast<QWidget *>(o);
            if ( w )
            {
                Parameter *p = wiz->GetParameters()[idx];
                QString sNew = p->GetValue( w );
                if ( !sNew.isNull() )
                {
                    sParams.append(sNew);
                    idx++;
                    if ( idx >= wiz->GetParameters().count() ) // Done.
                        break;
                }
            }
        }
    }
    return sParams;
}


void MainWindow::OnActionAccept()       // Accept current edits
{
    QModelIndex current = ui->lstWizFile->currentIndex();
    Wizard *wiz = (Wizard *)ui->treeWizards->currentItem()->data(0,Qt::UserRole).value<void *>();
    QStringList sParams = GetPanelParameters( wiz );
    
    if ( m_bEditing )
    {
        if ( !current.isValid() )
        {
            Q_ASSERT(false);
            return; 
        }
    
        WizLine * wl = static_cast<WizLine *>(current.data(WizLineRole).value<void *>());
        WizLineOWord *wlow = dynamic_cast<WizLineOWord *>(wl);
        Q_ASSERT( wlow );
    
        for ( int idx = 0; idx < sParams.count(); idx++ )
        {
            //bool bDirty = false;
            if ( wlow->GetParams()[idx] != sParams[idx] )
            {
                wlow->GetParams()[idx] = sParams[idx];
                m_gwiz.GetWizFile()->SetDirty( current.row() );
//                bDirty = true;
            }
        }
    }
    else
    {
        int row = 0;
        if ( current.isValid() )
            row = current.row()+1;
        m_gwiz.GetWizFile()->AddWizardRow( row, wiz, sParams );
        ui->lstWizFile->setCurrentIndex( ui->lstWizFile->model()->index( row, 0 ) );
    }
    
    ui->lstWizFile->setEnabled(true);
    ui->editLine->setEnabled(true);
    ui->lstWizFile->setFocus();
    UpdateButtonState();
    LoadPreview();
}

void MainWindow::OnActionReject()       // Reject current edits
{
    ui->lstWizFile->setEnabled( true );
    ui->editLine->setEnabled(true);
    QModelIndex idxCurrentRow = ui->lstWizFile->currentIndex();
    ui->lstWizFile->setCurrentIndex( QModelIndex() );
    ui->lstWizFile->setCurrentIndex( idxCurrentRow );
}

void MainWindow::EditCurrentWizard( bool bEditing )
{
    // Can't move off list while editing
    if ( ui->lstWizFile->isEnabled() )
    {
        m_bEditing = bEditing;
        ui->lstWizFile->setEnabled(false);
        ui->editLine->setEnabled(false);
        UpdateButtonState();
    }
}

void MainWindow::onMRUSelected(const QString &sFile)
{
    if ( !SaveChanges() )
        return;
    
    QFile file(sFile);
    if ( !file.exists() )
    {
        if ( QMessageBox::question(this,"Missing file", QString("The file, '{1}' no longer exists.  Remove it from the most recently used file list?").arg(sFile), QMessageBox::Yes | QMessageBox::No ) == QMessageBox::Yes )
        {
            m_MRU.remove( sFile );
        }
        return;
    }
    LoadWizFile( sFile );
}

void MainWindow::ShowWizFile()
{
    ui->lstWizFile->setModel( m_gwiz.GetWizFile() );    
    
    connect( ui->lstWizFile->selectionModel(), SIGNAL(currentChanged(const QModelIndex &,const QModelIndex &)), this, SLOT(OnListCurrentItemChanged(const QModelIndex &,const QModelIndex &)) );
    if ( ui->lstWizFile->model()->rowCount() > 0 )
    {
        ui->lstWizFile->setCurrentIndex( ui->lstWizFile->model()->index(0,0) );
    }
    
    if ( !m_gwiz.GetWizFile()->GetFilename().isEmpty() )
    {
        m_MRU.append( m_gwiz.GetWizFile()->GetFilename() );
    }
    
    LoadPreview(true);    
}

void MainWindow::LoadWizFile( const QString sFilename )
{
    // load file
    if ( !m_gwiz.LoadWizFile( sFilename ) )
    {
        Q_ASSERT(false);
    }
    
    ShowWizFile();
}

void MainWindow::LoadPreview(bool bResetView)
{
    QString s = m_gwiz.MakeGCode(true);
    QTemporaryFile tmpFile(QDir::tempPath() + QDir::separator() + "gwiz2.preview.XXXXXX.tmp");
    tmpFile.setAutoRemove( true );
    tmpFile.open();
    QTextStream str(&tmpFile);
    str << s;
    str.flush();
    
    int row = -1;
    if ( ui->lstWizFile->currentIndex().isValid() )
        row = ui->lstWizFile->currentIndex().row() + 1;
    
    m_glPreview->Load( tmpFile.fileName().toLocal8Bit().constData(), row, bResetView );
}

void MainWindow::UpdateButtonState()
{
    bool bListViewLineSelected = ui->lstWizFile->currentIndex().isValid();
    bool bWizLineSelected = false;
    if ( ui->lstWizFile->currentIndex().isValid() )
    {
        QVariant wiz = ui->lstWizFile->model()->data( ui->lstWizFile->currentIndex(), WizardRole );
        if ( wiz.isValid() )
        {
            bWizLineSelected = true;
        }
    }
    bool bEditingParam = !ui->lstWizFile->isEnabled();
    bool bWizSelected = false;
    if ( ui->treeWizards->currentItem() != NULL )
    {
        Wizard *twz = (Wizard *)ui->treeWizards->currentItem()->data(0,Qt::UserRole).value<void *>();
        if ( twz != NULL )
            bWizSelected = true;
    }
    bool bListItemNotAtTop = ui->lstWizFile->currentIndex().row() > 0;
    bool bListItmeNotAtBottom = ui->lstWizFile->currentIndex().row() >= 0 && ui->lstWizFile->currentIndex().row() < ui->lstWizFile->model()->rowCount() - 1;
    bool bHasPasteBuffer = !m_WizLineClipboard.isNull();
    
    // lst
    ui->actionUp->setEnabled( !bEditingParam && bListItemNotAtTop );
    ui->actionDown->setEnabled( !bEditingParam && bListItmeNotAtBottom );
    ui->actionInsertWizard->setEnabled( !bEditingParam && bWizSelected );
    ui->actionInsertBlank->setEnabled( !bEditingParam );
    
    // param panel
    // edit buttons
    ui->actionAccept->setEnabled( bEditingParam );
    ui->actionReject->setEnabled( bEditingParam );
    ui->actionEditWizard->setEnabled(!bEditingParam && bWizLineSelected);
    ui->actionSetWizard->setEnabled(false && bWizLineSelected);
    
    ui->actionPaste->setEnabled( bHasPasteBuffer );
    ui->actionDelete->setEnabled( bListViewLineSelected );
    ui->actionCut->setEnabled( bListViewLineSelected );
    ui->actionCopy->setEnabled( bListViewLineSelected );
}


/*

- Use separate defaults files  

- Build gcode...
    - todo - clean up % use in wiz
    - Better error line detection
        - map line number to .wiz file, or o<word> file.
    - Add log/msg window
        - show gcode errors.
        - msgs
- support script (external ex) commnds.
    - Can just be a "comment" format, eg
        - (GWizardExternalCommand: COMMAND arg1 arg2 arg3)
        - COMMAND must be unique like oword wizards are.
    - When building the gcode, we..
        - execute the command, 
        - capture stdout
        - wrap stdout in o<word> function 
        - and call from GCODE.
- Preview
    - Measure?
    - axis labels
    - adjust grid as zoom
        - adjust axis and axis labels too
 */
