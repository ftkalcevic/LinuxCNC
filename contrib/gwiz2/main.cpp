#include "stdafx.h"
#include "mainwindow.h"
#include <QApplication>

#include "gwizard.h"

ApplicationSettings *g_pAppSettings = NULL;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName( "FranksWorkshop" );
    a.setOrganizationDomain( "www.franksworkshop.com.au" );
    a.setApplicationName( "gwiz2" );
    a.setApplicationVersion("");
    
    GWizard gwiz;
    gwiz.ParseCommandLine();
    gwiz.Init();
            
    g_pAppSettings = new ApplicationSettings( gwiz.GetWizIniFile(), &a );
    
    MainWindow w(gwiz);
    w.show();
    
    return a.exec();
}
