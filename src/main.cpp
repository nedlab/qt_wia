#include "wia_demo.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WiaDemoDialog dialog;

    return dialog.exec();
}
