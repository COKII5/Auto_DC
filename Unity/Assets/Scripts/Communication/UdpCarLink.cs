using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using AutoDc.Protocol;

namespace AutoDc.Communication
{
    public sealed class UdpCarLink
    {
        private const int ReceiveTimeoutMs = 100;
        private const int ThreadJoinTimeoutMs = 500;
        private const int AnyLocalPort = 0;

        private readonly ConcurrentQueue<(CarTelemetry telemetry, long arrivalTimestamp)> _received =
            new ConcurrentQueue<(CarTelemetry telemetry, long arrivalTimestamp)>();
        private UdpClient _client;
        private Thread _receiveThread;
        private volatile bool _running;
        private volatile string _error;
        private int _corruptPackets;

        public bool IsOpen => _running;
        public string Error => _error;
        public int CorruptPackets => Volatile.Read(ref _corruptPackets);

        public bool TryOpen(string host, int port, out string failure)
        {
            Close();
            if (!IPAddress.TryParse(host, out IPAddress address))
            {
                failure = $"IP no valida: '{host}'";
                return false;
            }
            if (port <= IPEndPoint.MinPort || port > IPEndPoint.MaxPort)
            {
                failure = $"Puerto fuera de rango: {port}";
                return false;
            }
            try
            {
                _client = new UdpClient(AnyLocalPort);
                _client.Client.ReceiveTimeout = ReceiveTimeoutMs;
                _client.Connect(address, port);
            }
            catch (Exception exception)
            {
                _client?.Close();
                _client = null;
                failure = exception.Message;
                return false;
            }
            failure = "";
            _error = null;
            Interlocked.Exchange(ref _corruptPackets, 0);
            while (_received.TryDequeue(out _))
            {
            }
            _running = true;
            _receiveThread = new Thread(ReceiveLoop) { IsBackground = true, Name = nameof(UdpCarLink) };
            _receiveThread.Start();
            return true;
        }

        public void Close()
        {
            _running = false;
            _client?.Close();
            if (_receiveThread != null)
            {
                _receiveThread.Join(ThreadJoinTimeoutMs);
                _receiveThread = null;
            }
            _client = null;
        }

        public bool TrySend(byte[] datagram)
        {
            if (!_running || _client == null)
            {
                return false;
            }
            try
            {
                _client.Send(datagram, datagram.Length);
                return true;
            }
            catch (Exception exception)
            {
                _error = exception.Message;
                return false;
            }
        }

        public bool TryDequeue(out CarTelemetry telemetry, out long arrivalTimestamp)
        {
            bool dequeued = _received.TryDequeue(out (CarTelemetry telemetry, long arrivalTimestamp) entry);
            telemetry = entry.telemetry;
            arrivalTimestamp = entry.arrivalTimestamp;
            return dequeued;
        }

        private void ReceiveLoop()
        {
            IPEndPoint remote = null;
            while (_running)
            {
                byte[] datagram;
                try
                {
                    datagram = _client.Receive(ref remote);
                }
                catch (SocketException exception) when (IsTransient(exception.SocketErrorCode))
                {
                    continue;
                }
                catch (Exception exception) when (exception is SocketException || exception is ObjectDisposedException)
                {
                    if (_running)
                    {
                        _error = exception.Message;
                        _running = false;
                    }
                    return;
                }
                long arrivalTimestamp = Stopwatch.GetTimestamp();
                if (CarTelemetry.TryDecode(datagram, datagram.Length, out CarTelemetry telemetry))
                {
                    _received.Enqueue((telemetry, arrivalTimestamp));
                }
                else
                {
                    Interlocked.Increment(ref _corruptPackets);
                }
            }
        }

        private static bool IsTransient(SocketError errorCode)
        {
            return errorCode == SocketError.TimedOut || errorCode == SocketError.ConnectionReset;
        }
    }
}
