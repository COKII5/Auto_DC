using System;
using System.Collections.Generic;
using System.Diagnostics;
using AutoDc.Protocol;

namespace AutoDc.Communication
{
    public sealed class LinkMonitor
    {
        private const int HistorySize = 256;
        private const double NoResponseSeconds = 0.5;
        private const double RateWindowSeconds = 1.0;
        private const double RttSmoothing = 0.1;
        private const double MillisecondsPerSecond = 1000.0;

        private readonly ushort[] _sentSequences = new ushort[HistorySize];
        private readonly long[] _sentTimestamps = new long[HistorySize];
        private readonly Queue<long> _recentArrivals = new Queue<long>();
        private long _lastArrivalTimestamp;

        public bool HasTelemetry { get; private set; }
        public CarTelemetry LastTelemetry { get; private set; }
        public double LastRttMs { get; private set; }
        public double SmoothedRttMs { get; private set; }
        public int TelemetryRateHz => _recentArrivals.Count;

        public void Reset()
        {
            HasTelemetry = false;
            LastTelemetry = default;
            LastRttMs = 0.0;
            SmoothedRttMs = 0.0;
            _recentArrivals.Clear();
            Array.Clear(_sentTimestamps, 0, HistorySize);
        }

        public void RecordSent(ushort sequence, long timestamp)
        {
            int slot = sequence % HistorySize;
            _sentSequences[slot] = sequence;
            _sentTimestamps[slot] = timestamp;
        }

        public void RecordTelemetry(CarTelemetry telemetry, long arrivalTimestamp)
        {
            LastTelemetry = telemetry;
            HasTelemetry = true;
            _lastArrivalTimestamp = arrivalTimestamp;
            _recentArrivals.Enqueue(arrivalTimestamp);
            UpdateRtt(telemetry, arrivalTimestamp);
        }

        public LinkState Evaluate(bool linkOpen, long now)
        {
            DropOldArrivals(now);
            if (!linkOpen)
            {
                return LinkState.Disconnected;
            }
            if (!HasTelemetry)
            {
                return LinkState.WaitingForCar;
            }
            return SecondsBetween(_lastArrivalTimestamp, now) > NoResponseSeconds ? LinkState.NoResponse : LinkState.Connected;
        }

        private void UpdateRtt(CarTelemetry telemetry, long arrivalTimestamp)
        {
            int slot = telemetry.AckSequence % HistorySize;
            if (_sentTimestamps[slot] == 0 || _sentSequences[slot] != telemetry.AckSequence)
            {
                return;
            }
            double rttMs = SecondsBetween(_sentTimestamps[slot], arrivalTimestamp) * MillisecondsPerSecond - telemetry.AckAgeMs;
            LastRttMs = rttMs < 0.0 ? 0.0 : rttMs;
            SmoothedRttMs = SmoothedRttMs == 0.0 ? LastRttMs : SmoothedRttMs + RttSmoothing * (LastRttMs - SmoothedRttMs);
        }

        private void DropOldArrivals(long now)
        {
            while (_recentArrivals.Count > 0 && SecondsBetween(_recentArrivals.Peek(), now) > RateWindowSeconds)
            {
                _recentArrivals.Dequeue();
            }
        }

        private static double SecondsBetween(long start, long end)
        {
            return (double)(end - start) / Stopwatch.Frequency;
        }
    }
}
