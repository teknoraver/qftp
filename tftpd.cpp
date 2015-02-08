#include <QtEndian>
#include <QFile>

#include <tftpd.h>

Tftpd::Tftpd()
{
	start();
}

void Tftpd::run()
{
	sock.bind(69);
	while(1) {
		sock.waitForReadyRead(-1);
		qint64 readed = sock.readDatagram(buffer, SEGSIZE, &rhost, &rport);
		if(readed < 0)
			continue;

		struct tftp_header *th = (struct tftp_header *)buffer;
		th->opcode = qFromBigEndian(th->opcode);
		switch(th->opcode) {
		case RRQ:
			sendfile(th);
			break;
		case WRQ:
			getfile(th);
			break;
		default: nak(EBADOP);
		}
	}
}

void Tftpd::sendfile(struct tftp_header *th)
{
	char *filename = th->path;
	qDebug("sending %s", filename);
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
		switch (file.error()) {
		case QFile::OpenError:
			nak(ENOTFOUND);
			return;
		case QFile::PermissionsError:
			nak(EACCESS);
			return;
		default:
			nak(EUNDEF);
			return;
		}

	quint64 readed;
	quint16 block = 1;
	do {
		th->opcode = qToBigEndian((quint16)DATA);
		th->data.block = qToBigEndian((quint16)block);
		readed = file.read(buffer + sizeof(struct tftp_header), SEGSIZE);

		sock.writeDatagram(buffer, readed + sizeof(struct tftp_header), rhost, rport);

		block++;
		th->data.block = qToBigEndian((quint16)block);

		while(1) {
			QHostAddress h;
			quint16 p;
			sock.readDatagram(buffer, SEGSIZE, &h, &p);
			if(h != rhost || p != rport)
				continue;

			if(qFromBigEndian(th->opcode) == ACK && qFromBigEndian(th->data.block) == block - 1)
				break;
		}
	} while(readed == SEGSIZE);
	qDebug("sent %d blocks, %llu bytes", (block - 1), (block - 2) * SEGSIZE + readed);
}

void Tftpd::getfile(struct tftp_header *th)
{
	char *filename = th->path;
	qDebug("receiving %s", filename);
	QFile file(filename);
	if(!file.open(QIODevice::WriteOnly))
		switch (file.error()) {
		case QFile::PermissionsError:
			nak(EACCESS);
			return;
		default:
			nak(EUNDEF);
			return;
		}

	ack(0);
	quint64 received;
	quint16 block = 1;
	do {
		while(1) {
			QHostAddress h;
			quint16 p;
			sock.waitForReadyRead(-1);
			received = sock.readDatagram(buffer, SEGSIZE + sizeof(struct tftp_header), &h, &p);

			if(h != rhost || p != rport)
				continue;

			if(qFromBigEndian(th->opcode) == DATA && qFromBigEndian(th->data.block) == block)
				break;
		}
		file.write(buffer + sizeof(struct tftp_header), received - sizeof(struct tftp_header));
		ack(block++);
	} while (received == SEGSIZE + sizeof(struct tftp_header));
	qDebug("received %d blocks, %llu bytes", block - 1, (block - 2) * SEGSIZE + received);
}

void Tftpd::ack(quint16 block)
{
	struct tftp_header ack;
	ack.opcode = qToBigEndian((quint16)ACK);
	ack.data.block = qToBigEndian(block);
	sock.writeDatagram((char*)&ack, sizeof(struct tftp_header), rhost, rport);
}

void Tftpd::nak(Error error)
{
	struct tftp_header *th = (struct tftp_header *)buffer;
	th->opcode = qToBigEndian((quint16)ERROR);
	th->data.block = qToBigEndian((quint16)error);

	struct errmsg *pe;
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = strerror(error - 100);
		th->data.block = EUNDEF;   /* set 'undef' errorcode */
	}

	strcpy(th->data.data, pe->e_msg);
	int length = strlen(pe->e_msg);
	th->data.data[length] = 0;
	length += 5;
	sock.writeDatagram(buffer, length, rhost, rport);
}
