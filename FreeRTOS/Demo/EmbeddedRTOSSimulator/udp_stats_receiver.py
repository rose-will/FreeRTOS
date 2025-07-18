import socket
import csv
import time

UDP_IP = "0.0.0.0"  # Listen on all interfaces
UDP_PORT = 5005      # Match REMOTE_STATS_PORT in main.c
CSV_FILE = "udp_stats_log.csv"

print(f"Listening for UDP stats on {UDP_IP}:{UDP_PORT} ...")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

# Open CSV file for appending
with open(CSV_FILE, 'a', newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(["timestamp", "heap", "q1", "q2", "sem", "ev"])
    while True:
        data, addr = sock.recvfrom(1024)
        line = data.decode().strip()
        print(f"Received from {addr}: {line}")
        # Parse line like: HEAP:12345 Q1:2 Q2:1 SEM:0 EV:0x00000001
        try:
            parts = dict(part.split(":") for part in line.replace("EV:", "EV:").split() if ":" in part)
            heap = int(parts.get("HEAP", 0))
            q1 = int(parts.get("Q1", 0))
            q2 = int(parts.get("Q2", 0))
            sem = int(parts.get("SEM", 0))
            ev = parts.get("EV", "0")
            writer.writerow([int(time.time()), heap, q1, q2, sem, ev])
            csvfile.flush()
        except Exception as e:
            print(f"Parse error: {e}")