#include <QMessageBox>
#include <QDir>
#include <QFileDialog>

#include "qtftpgui.h"

QTftpGui::QTftpGui()
{
	setupUi(this);
	connect(actionAbout, SIGNAL(triggered()), SLOT(about()));
	connect(actionAbout_Qt, SIGNAL(triggered()), QCoreApplication::instance(), SLOT(aboutQt()));

	connect(start, SIGNAL(clicked()), SLOT(startServer()));
	connect(browse, SIGNAL(clicked()), SLOT(setRoot()));

	connect(serverip, SIGNAL(textChanged(QString)), SLOT(enablePutGet(QString)));
	connect(root, SIGNAL(textChanged(QString)), SLOT(enableServer(QString)));

	connect(&qtftp, SIGNAL(fileSent(QString)), SLOT(sent(QString)));
	connect(&qtftp, SIGNAL(fileReceived(QString)), SLOT(received(QString)));
	connect(get, SIGNAL(clicked()), SLOT(getFile()));
	connect(put, SIGNAL(clicked()), SLOT(putFile()));
	connect(&qtftp, SIGNAL(error(int)), SLOT(error(int)));
	connect(&qtftp, SIGNAL(send(bool)), progressBar, SLOT(setEnabled(bool)));
	connect(&qtftp, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
}


void QTftpGui::startServer()
{
	bool running = qtftp.isRunning();
	root->setEnabled(running);
	browse->setEnabled(running);
	if(running) {
		qtftp.stopServer();
		start->setText("&Start Server");
		statusbar->showMessage("Server stopped");
	} else {
		qtftp.startServer();
		start->setText("&Stop Server");
		statusbar->showMessage("Server started");
	}
	clientgroup->setEnabled(running);
}

void QTftpGui::setRoot()
{
	QString path = QFileDialog::getExistingDirectory(0, QString(), root->text());
	if(path.length() && QDir(path).exists()) {
		QDir::setCurrent(path);
		root->setText(path);
	}
}

void QTftpGui::sent(QString file)
{
	statusbar->showMessage("sent '" + file + "'");
	progressBar->reset();
	progressBar->setEnabled(false);
	servergroup->setEnabled(true);
	clientgroup->setEnabled(true);
}

void QTftpGui::received(QString file)
{
	statusbar->showMessage("received '" + file + "'");
	servergroup->setEnabled(true);
	clientgroup->setEnabled(true);
}

void QTftpGui::putFile()
{
	QString path = QFileDialog::getOpenFileName();
	if(path.length() && QFile::exists(path)) {
		servergroup->setEnabled(false);
		clientgroup->setEnabled(false);
		progressBar->setEnabled(true);
		statusbar->showMessage("sending '" + path + "'");
		qtftp.put(path, serverip->text());
	}
}

void QTftpGui::getFile()
{
	QString path = QFileDialog::getSaveFileName();
	if(path.length()) {
		servergroup->setEnabled(false);
		clientgroup->setEnabled(false);
		statusbar->showMessage("receiving '" + path + "'");
		qtftp.get(path, serverip->text());
	}
}

void QTftpGui::enableServer(QString text)
{
	QDir dir(text);
	start->setEnabled(dir.exists());
}

void QTftpGui::enablePutGet(QString text)
{
	get->setEnabled(!text.isEmpty());
	put->setEnabled(!text.isEmpty());
}

void QTftpGui::error(int err)
{
	QString errorMsg;
	switch (err) {
	case QTftp::Ok:
		return;
	case QTftp::Timeout:
		errorMsg = "Operation timed out";
		break;
	case QTftp::BindError:
		errorMsg = "Failed to bind socket\nMaybe it's already bound or you need root privileges";
		break;
	case QTftp::NetworkError:
		errorMsg = "Network Error";
		break;
	default:
		errorMsg = "Error";
		break;
	}
	QMessageBox::critical(this, "Error", errorMsg);
	servergroup->setEnabled(true);
	clientgroup->setEnabled(true);
}

void QTftpGui::about()
{
	QMessageBox::about(this, "About QTftpGui", "QTftpGui - a Qt TFTP implementation<br>by Matteo Croce <a href=\"http://teknoraver.net/\">http://teknoraver.net/</a>");
}
