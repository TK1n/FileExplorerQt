#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QTreeView>
#include <QTableView>
#include <QModelIndex>
#include <QAction>
#include <QMenu>
#include <QClipboard>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void onTreeClicked(const QModelIndex &index);
    void createNewFolder();
    void renameItem();
    void deleteItem();
    void copyItem();
    void pasteItem();
    void showProperties();

private:
    QFileSystemModel *dirModel;
    QFileSystemModel *fileModel;
    QTreeView *treeView;
    QTableView *tableView;

    QString copiedPath;   // For copy/paste
    bool copyIsCut = false; // true if Cut (Move), false if Copy
};
#endif // MAINWINDOW_H
