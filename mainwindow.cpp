#include "mainwindow.h"
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(900, 600);

    // === MODELS ===
    dirModel = new QFileSystemModel(this);
    dirModel->setRootPath("");
    dirModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);   // only folders/drives

    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath("");
    fileModel->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries); // files + folders

    // === VIEWS ===
    treeView = new QTreeView(this);
    treeView->setModel(dirModel);
    treeView->setRootIndex(QModelIndex()); // show drives at top level
    treeView->setHeaderHidden(true);
    treeView->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    treeView->setColumnWidth(0, 250);
    // treeView->hideColumn(1); // hide Size
    // treeView->hideColumn(2); // hide Type
    // treeView->hideColumn(3); // hide Date Modified

    tableView = new QTableView(this);
    tableView->setModel(fileModel);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    // === SYNC: tree → table ===
    connect(treeView, &QTreeView::clicked, this, &MainWindow::onTreeClicked);

    // === SYNC: table double-click → table+tree ===
    connect(tableView, &QTableView::doubleClicked, this, [this](const QModelIndex &index){
        QString path = fileModel->filePath(index);
        QFileInfo info(path);

        if (info.isDir()) {
            // navigate into folder in table
            tableView->setRootIndex(fileModel->setRootPath(path));
            statusBar()->showMessage(path);

            // sync tree
            QModelIndex treeIndex = dirModel->index(path);
            if (treeIndex.isValid()) {
                treeView->setCurrentIndex(treeIndex);
                treeView->expand(treeIndex);
                treeView->scrollTo(treeIndex);
            }
        } else {
            // open file in default app
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });

    // === LAYOUT ===
    QSplitter *splitter = new QSplitter(this);
    splitter->addWidget(treeView);
    splitter->addWidget(tableView);
    setCentralWidget(splitter);

    // === MENUS & TOOLBAR ===
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("New Folder", this, &MainWindow::createNewFolder);
    fileMenu->addAction("Rename", this, &MainWindow::renameItem);
    fileMenu->addAction("Delete", this, &MainWindow::deleteItem);
    fileMenu->addAction("Copy", this, &MainWindow::copyItem);
    fileMenu->addAction("Paste", this, &MainWindow::pasteItem);
    fileMenu->addAction("Properties", this, &MainWindow::showProperties);

    // Navigation menu + toolbar
    QAction *upAction = new QAction(QIcon::fromTheme("go-up"), "Up", this);
    connect(upAction, &QAction::triggered, this, [this](){
        QModelIndex currentIndex = tableView->rootIndex();
        if (!currentIndex.isValid()) return;

        QString currentPath = fileModel->filePath(currentIndex);
        QDir dir(currentPath);
        if (dir.cdUp()) {
            QString parentPath = dir.absolutePath();
            tableView->setRootIndex(fileModel->setRootPath(parentPath));

            // sync tree
            QModelIndex treeIndex = dirModel->index(parentPath);
            if (treeIndex.isValid()) {
                treeView->setCurrentIndex(treeIndex);
                treeView->expand(treeIndex);
                treeView->scrollTo(treeIndex);
            }

            statusBar()->showMessage(parentPath);
        }
    });

    QMenu *navMenu = menuBar()->addMenu("&Navigate");
    navMenu->addAction(upAction);

    QToolBar *toolbar = addToolBar("Navigation");
    toolbar->addAction(upAction);

    // === STATUS BAR ===
    statusBar()->showMessage("Ready");
}

void MainWindow::onTreeClicked(const QModelIndex &index) {
    QString path = dirModel->fileInfo(index).absoluteFilePath();

    // If it's a drive or folder → set table root to show its contents
    if (dirModel->fileInfo(index).isDir()) {
        tableView->setRootIndex(fileModel->setRootPath(path));
    }
    // If it's a file → just show its parent directory
    else {
        QString parentPath = dirModel->fileInfo(index).absolutePath();
        tableView->setRootIndex(fileModel->setRootPath(parentPath));
        // Optionally: auto-select the file in table
        QModelIndex childIndex = fileModel->index(path);
        if (childIndex.isValid()) {
            tableView->selectRow(childIndex.row());
        }
    }

    statusBar()->showMessage(path);
}
void MainWindow::createNewFolder() {
    QModelIndex index = tableView->rootIndex();
    QString path = fileModel->filePath(index);
    if (path.isEmpty()) path = QDir::homePath();

    bool ok;
    QString folderName = QInputDialog::getText(this, "New Folder",
                                               "Folder Name:", QLineEdit::Normal,
                                               "New Folder", &ok);
    if (ok && !folderName.isEmpty()) {
        QDir dir(path);
        if (!dir.mkdir(folderName)) {
            QMessageBox::warning(this, "Error", "Failed to create folder.");
        }
    }
}

void MainWindow::renameItem() {
    QModelIndex index = tableView->currentIndex();
    if (!index.isValid()) return;

    QString oldPath = fileModel->filePath(index);
    QFileInfo info(oldPath);

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename",
                                            "New name:", QLineEdit::Normal,
                                            info.fileName(), &ok);
    if (ok && !newName.isEmpty()) {
        QString newPath = info.dir().absolutePath() + "/" + newName;
        if (!QFile::rename(oldPath, newPath)) {
            QMessageBox::warning(this, "Error", "Failed to rename.");
        }
    }
}

void MainWindow::deleteItem() {
    QModelIndex index = tableView->currentIndex();
    if (!index.isValid()) return;

    QString path = fileModel->filePath(index);
    QFileInfo info(path);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete", "Are you sure you want to delete " + info.fileName() + "?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (info.isDir()) {
            QDir dir(path);
            if (!dir.removeRecursively()) {
                QMessageBox::warning(this, "Error", "Failed to delete folder.");
            }
        } else {
            if (!QFile::remove(path)) {
                QMessageBox::warning(this, "Error", "Failed to delete file.");
            }
        }
    }
}

void MainWindow::copyItem() {
    QModelIndex index = tableView->currentIndex();
    if (!index.isValid()) return;
    copiedPath = fileModel->filePath(index);
    copyIsCut = false; // Copy, not move
    statusBar()->showMessage("Copied: " + copiedPath);
}

bool copyRecursively(const QString &srcPath, const QString &dstPath) {
    QDir srcDir(srcPath);
    if (!srcDir.exists())
        return false;

    QDir dstDir(dstPath);
    if (!dstDir.exists()) {
        if (!dstDir.mkpath(".")) {
            return false;
        }
    }

    // Copy files
    for (const QFileInfo &fileInfo : srcDir.entryInfoList(QDir::Files)) {
        QString srcFilePath = fileInfo.absoluteFilePath();
        QString dstFilePath = dstDir.filePath(fileInfo.fileName());
        if (QFile::exists(dstFilePath)) {
            QFile::remove(dstFilePath); // overwrite
        }
        if (!QFile::copy(srcFilePath, dstFilePath)) {
            return false;
        }
    }

    // Copy subdirectories
    for (const QFileInfo &dirInfo : srcDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString srcSubDir = dirInfo.absoluteFilePath();
        QString dstSubDir = dstDir.filePath(dirInfo.fileName());
        if (!copyRecursively(srcSubDir, dstSubDir)) {
            return false;
        }
    }

    return true;
}

void MainWindow::pasteItem() {
    if (copiedPath.isEmpty()) return;

    QModelIndex index = tableView->rootIndex();
    QString destDir = fileModel->filePath(index);
    QFileInfo info(copiedPath);
    QString newPath = destDir + "/" + info.fileName();

        if (info.isDir()) {
        if (!copyRecursively(copiedPath, newPath)) {
            QMessageBox::warning(this, "Error", "Failed to copy folder.");
            return;
        }
        if (copyIsCut) {
            QDir dir(copiedPath);
            dir.removeRecursively();
        }
    } else {
        if (QFile::exists(newPath)) {
            QFile::remove(newPath); // overwrite
        }
        if (!QFile::copy(copiedPath, newPath)) {
            QMessageBox::warning(this, "Error", "Failed to copy file.");
            return;
        }
        if (copyIsCut) {
            QFile::remove(copiedPath);
        }
    }

    statusBar()->showMessage("Pasted to: " + destDir);
}

void MainWindow::showProperties() {
    QModelIndex index = tableView->currentIndex();
    if (!index.isValid()) return;

    QString path = fileModel->filePath(index);
    QFileInfo info(path);

    QString details;
    details += "Name: " + info.fileName() + "\n";
    details += "Path: " + info.absoluteFilePath() + "\n";
    details += "Size: " + QString::number(info.size() / 1024.0, 'f', 2) + " KB\n";
    // details += "Type: " + (info.isDir() ? "Folder" : "File") + "\n";
    details += QString("Type: %1\n").arg(info.isDir() ? "Folder" : "File");
    details += "Last Modified: " + info.lastModified().toString() + "\n";

    QMessageBox::information(this, "Properties", details);
}
