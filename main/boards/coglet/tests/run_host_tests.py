#!/usr/bin/env python3
"""Compile the real controller against fake IDF I/O; never opens a device."""
from pathlib import Path
import os
import json
import subprocess
import tempfile

board = Path(__file__).resolve().parents[1]
idf = Path(os.environ.get('IDF_PATH', str(Path.home() / 'esp/esp-idf')))
with tempfile.TemporaryDirectory(prefix='coglet-tests-') as temporary:
    tmp = Path(temporary)
    for name in ('driver/i2c_master.h', 'driver/gpio.h', 'esp_http_server.h',
                 'esp_log.h', 'esp_random.h', 'esp_timer.h', 'esp_netif.h',
                 'nvs.h', 'freertos/FreeRTOS.h', 'freertos/task.h', 'mcp_server.h'):
        path = tmp / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "fake_idf.h"\n')
    (tmp / 'fake_idf.h').write_text((board / 'tests/fake_idf.h').read_text())
    cjson = idf / 'components/json/cJSON'
    subprocess.run(['cc', '-c', str(cjson / 'cJSON.c'), '-I'+str(cjson), '-o', str(tmp/'cjson.o')], check=True)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                    '-I'+str(tmp), '-I'+str(board), '-I'+str(cjson),
                    str(board/'coglet_controller.cc'), str(board/'tests/test_controller.cc'),
                    str(tmp/'cjson.o'), '-pthread', '-o', str(tmp/'test')], check=True)
    export = tmp / 'calibration.json'
    subprocess.run([str(tmp/'test'), str(export)], check=True)
    restore = board / 'tools/restore_commands.py'
    commands = subprocess.check_output(['python3', str(restore), str(export)], text=True)
    assert commands.startswith('!coglet release\n')
    assert commands.endswith('!coglet save\n!coglet export\n')
    assert 'engage' not in commands and 'servo ' not in commands
    data = json.loads(export.read_text())
    data['axes'][1]['channel'] = data['axes'][0]['channel']
    export.write_text(json.dumps(data))
    bad = subprocess.run(['python3', str(restore), str(export)], capture_output=True)
    assert bad.returncode and not bad.stdout
    print('Calibration restore exporter accepted valid data and rejected duplicate mapping.')
