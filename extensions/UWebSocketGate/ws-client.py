#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Простой нагрузочный клиент для UWebSocketGate.

Пример: запросить 1000 датчиков 100000-100999 и слушать 30 секунд
    python ws-client.py --ws ws://localhost:8081/wsgate --start-id 100000 --count 1000 --duration 30
"""
import argparse
import json
import time
from typing import Tuple

from websocket import create_connection, WebSocketTimeoutException


def build_ask_command(start_id: int, count: int) -> str:
    ids = [str(start_id + i) for i in range(count)]
    return "ask:" + ",".join(ids)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="UWebSocketGate load client")
    p.add_argument("--ws", default="ws://localhost:8081/wsgate", help="WS URL (default: ws://localhost:8081/wsgate)")
    p.add_argument("--start-id", type=int, default=0, help="Первый ID датчика")
    p.add_argument("--count", type=int, default=1000, help="Сколько датчиков подписать")
    p.add_argument("--duration", type=int, default=30, help="Длительность приёма, сек")
    p.add_argument("--recv-timeout", type=float, default=5.0, help="Таймаут recv, сек")
    p.add_argument("--verbose", action="store_true", help="Печать первых кадров")
    p.add_argument("--check-responses", action="store_true",
                   help="Проверять, что каждый датчик из диапазона прислал хотя бы одно обновление")
    return p.parse_args()


def stats(frames: int, updates: int, duration: float) -> Tuple[float, float]:
    if duration <= 0:
        return 0.0, 0.0
    return frames / duration, updates / duration


def main() -> int:
    args = parse_args()

    ws = create_connection(args.ws, timeout=args.recv_timeout)
    cmd = build_ask_command(args.start_id, args.count)
    ws.send(cmd)

    total_frames = 0
    total_updates = 0
    start_ts = time.time()
    end_ts = start_ts + args.duration
    received_ids = set()

    try:
        while True:
            now = time.time()
            if now >= end_ts:
                break

            try:
                frame = ws.recv()
            except WebSocketTimeoutException:
                continue

            if not frame or frame == ".":
                continue

            total_frames += 1
            if args.verbose and total_frames <= 5:
                print(frame)

            try:
                data = json.loads(frame)
                if isinstance(data, dict) and "data" in data and isinstance(data["data"], list):
                    total_updates += len(data["data"])
                    if args.check_responses:
                        for item in data["data"]:
                            if isinstance(item, dict) and "id" in item:
                                try:
                                    received_ids.add(int(item["id"]))
                                except Exception:
                                    pass
            except Exception:
                # оставляем для подсчёта кадров, но без деталей
                pass

    finally:
        ws.close()

    duration = max(0.001, time.time() - start_ts)
    fps, ups = stats(total_frames, total_updates, duration)

    print(f"WS: {args.ws}")
    print(f"Subscribed sensors: {args.count} (from {args.start_id})")
    print(f"Duration: {duration:.2f}s")
    print(f"Frames: {total_frames} ({fps:.2f}/s)")
    print(f"Updates: {total_updates} ({ups:.2f}/s)")
    if args.check_responses:
        expected_ids = set(range(args.start_id, args.start_id + args.count))
        missing = sorted(expected_ids - received_ids)
        if missing:
            print(f"Missing updates from {len(missing)} sensors: {missing[:20]}{'...' if len(missing) > 20 else ''}")
        else:
            print("All sensors responded at least once.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
