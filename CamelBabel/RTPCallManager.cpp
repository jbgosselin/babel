#include <QtEndian>
#include <functional>
#include "RTPCallManager.hpp"
#include "rtp.hpp"

RTPCallManager::RTPCallManager(QObject *parent) :
  QObject(parent)
{
  using namespace std::placeholders;
  auto portAudio = createPortAudio(44100, 256, std::bind(&RTPCallManager::handleAudio, this, _1, _2, _3, _4, _5));

  portAudio.start();
  Pa_Sleep(10000);
  portAudio.stop();
  initSocket();
  t_rtp rtp;
  rtp.rtp_ver = 2;
  rtp.rtp_pad = 0;
  rtp.rtp_ext = 0;
  rtp.rtp_cc = 0;
  rtp.rtp_marker = 0;
  rtp.rtp_type = 10;
  rtp.rtp_seq = qToBigEndian<quint16>(1);
  rtp.rtp_ts = qToBigEndian<quint32>(1);
  rtp.rtp_ssrc = qToBigEndian<quint32>(42);
  sendDatagram(QByteArray((char*)&rtp, sizeof(rtp)));
}

void RTPCallManager::sendDatagram(const QByteArray &datagram)
{
  _udpSocket->writeDatagram(datagram.data(), datagram.size(),
                            _contact, _contactPort);
}

void RTPCallManager::call()
{

}

void RTPCallManager::finishCall()
{

}

void RTPCallManager::resumePlaying()
{

}

void RTPCallManager::readPendingDatagrams()
{
    while (_udpSocket->hasPendingDatagrams())
      {
        QByteArray      datagram;
        QHostAddress    sender;
        quint16         senderPort;

        datagram.resize(_udpSocket->pendingDatagramSize());
        _udpSocket->readDatagram(datagram.data(), datagram.size(),
                                 &sender, &senderPort);
        if (sender == _contact && senderPort == _contactPort)
          processRTPDatagram(datagram);
    }
}

void RTPCallManager::initSocket()
{
    _udpSocket = new QUdpSocket(this);
    _udpSocket->bind(QHostAddress::Any, 4242);
    connect(_udpSocket, SIGNAL(readyRead()),
            this, SLOT(readPendingDatagrams()));
    qDebug() << _udpSocket->localPort();
}

void RTPCallManager::processRTPDatagram(QByteArray &datagram)
{
  t_rtp         *rtp = reinterpret_cast<t_rtp*>(datagram.data());
  size_t        rtpHeaderLength;

  qDebug() << "sizeof(*rtp) = " << sizeof(*rtp);
  qDebug() << "START PROCESSING RTP Datagram";
  qDebug() << datagram;
  qDebug() << "datagram.size = " << datagram.size();
  qDebug() << "rtp->rtp_ver = " << rtp->rtp_ver;
  qDebug() << "rtp->rtp_seq = " << qFromBigEndian(rtp->rtp_seq);
  qDebug() << "rtp->rtp_ssrc = " << qFromBigEndian(rtp->rtp_ssrc);
  if (static_cast<size_t>(datagram.size()) < sizeof(*rtp)
      || rtp->rtp_ver != 2
      || rtp->rtp_cc > 15
      || qFromBigEndian(rtp->rtp_ts) == 0
      || rtp->rtp_type > 34)
    return;
  rtpHeaderLength = RTP_LENGTH + rtp->rtp_cc * 4;
  if (rtp->rtp_ext)
    {
      t_rtpx *rtpx = (t_rtpx *)(datagram.data() + rtpHeaderLength);
      rtpHeaderLength += (qFromBigEndian(rtpx->rtpx_len) * 4) + RTPX_LENGTH;
    }
  if (rtp->rtp_pad)
    datagram.truncate(datagram.data()[datagram.size() - 1]);
  qDebug() << "FINISH PROCESSING RTP Datagram";
  qDebug() << "PAYLOAD:" << &datagram.data()[rtpHeaderLength];
}

void RTPCallManager::startConversation()
{

}

void RTPCallManager::stopConversation()
{

}

void RTPCallManager::handleAudio(const float *input, float *output,
				 unsigned long frameCount,
				 const PaStreamCallbackTimeInfo *timeInfo,
				 PaStreamCallbackFlags statusFlags)
{
  (void)timeInfo;
  (void)statusFlags;
  for (unsigned long i = 0; i < frameCount; ++i)
    *output++ = *input++;
}