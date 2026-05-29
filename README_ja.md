# MCP ベースのチャットボット

（日本語 | [中文](README_zh.md) | [English](README.md)）

## はじめに

👉 [人間：AIにカメラを装着 vs AI：その場で飼い主が3日間髪を洗っていないことを発見【bilibili】](https://www.bilibili.com/video/BV1bpjgzKEhd/)

👉 [手作りでAIガールフレンドを作る、初心者入門チュートリアル【bilibili】](https://www.bilibili.com/video/BV1XnmFYLEJN/)

シャオジーAIチャットボットは音声インタラクションの入口として、Qwen / DeepSeekなどの大規模モデルのAI能力を活用し、MCPプロトコルを通じてマルチエンド制御を実現します。

<img src="docs/mcp-based-graph.jpg" alt="MCPであらゆるものを制御" width="320">

## バージョンノート

現在のv2バージョンはv1パーティションテーブルと互換性がないため、v1からv2へOTAでアップグレードすることはできません。パーティションテーブルの詳細については、[partitions/v2/README.md](partitions/v2/README.md)をご参照ください。

v1を実行しているすべてのハードウェアは、ファームウェアを手動で書き込むことでv2にアップグレードできます。

v1の安定版は1.9.2です。`git checkout v1`でv1に切り替えることができます。v1ブランチは2026年2月まで継続的にメンテナンスされます。

### 実装済み機能

- Wi-Fi / ML307 Cat.1 4G
- オフライン音声ウェイクアップ [ESP-SR](https://github.com/espressif/esp-sr)
- 2種類の通信プロトコルに対応（[Websocket](docs/websocket.md) または MQTT+UDP）
- OPUSオーディオコーデックを採用
- ストリーミングASR + LLM + TTSアーキテクチャに基づく音声インタラクション
- 話者認識、現在話している人を識別 [3D Speaker](https://github.com/modelscope/3D-Speaker)
- OLED / LCDディスプレイ、表情表示対応
- バッテリー表示と電源管理
- 多言語対応（中国語、英語、日本語）
- ESP32-C3、ESP32-S3、ESP32-P4チッププラットフォーム対応
- デバイス側MCPによるデバイス制御（音量・明るさ調整、アクション制御など）
- クラウド側MCPで大規模モデル能力を拡張（スマートホーム制御、PCデスクトップ操作、知識検索、メール送受信など）
- カスタマイズ可能なウェイクワード、フォント、絵文字、チャット背景、オンラインWeb編集に対応 ([カスタムアセットジェネレーター](https://github.com/78/xiaozhi-assets-generator))

## ハードウェア

### ブレッドボード手作り実践

Feishuドキュメントチュートリアルをご覧ください：

👉 [「シャオジーAIチャットボット百科事典」](https://ccnphfhqs21z.feishu.cn/wiki/F5krwD16viZoF0kKkvDcrZNYnhb?from=from_copylink)

ブレッドボードのデモ：

![ブレッドボードデモ](docs/v1/wiring2.jpg)

### 70種類以上のオープンソースハードウェアに対応（一部のみ表示）

- <a href="https://oshwhub.com/li-chuang-kai-fa-ban/li-chuang-shi-zhan-pai-esp32-s3-kai-fa-ban" target="_blank" title="立創・実戦派 ESP32-S3 開発ボード">立創・実戦派 ESP32-S3 開発ボード</a>
- <a href="https://github.com/espressif/esp-box" target="_blank" title="楽鑫 ESP32-S3-BOX3">楽鑫 ESP32-S3-BOX3</a>
- <a href="https://docs.m5stack.com/zh_CN/core/CoreS3" target="_blank" title="M5Stack CoreS3">M5Stack CoreS3</a>
- <a href="https://docs.m5stack.com/en/atom/Atomic%20Echo%20Base" target="_blank" title="AtomS3R + Echo Base">M5Stack AtomS3R + Echo Base</a>
- <a href="https://gf.bilibili.com/item/detail/1108782064" target="_blank" title="マジックボタン2.4">マジックボタン2.4</a>
- <a href="https://www.waveshare.net/shop/ESP32-S3-Touch-AMOLED-1.8.htm" target="_blank" title="微雪電子 ESP32-S3-Touch-AMOLED-1.8">微雪電子 ESP32-S3-Touch-AMOLED-1.8</a>
- <a href="https://github.com/Xinyuan-LilyGO/T-Circle-S3" target="_blank" title="LILYGO T-Circle-S3">LILYGO T-Circle-S3</a>
- <a href="https://oshwhub.com/tenclass01/xmini_c3" target="_blank" title="エビ兄さん Mini C3">エビ兄さん Mini C3</a>
- <a href="https://oshwhub.com/movecall/cuican-ai-pendant-lights-up-y" target="_blank" title="Movecall CuiCan ESP32S3">CuiCan AIペンダント</a>
- <a href="https://github.com/WMnologo/xingzhi-ai" target="_blank" title="無名科技Nologo-星智-1.54">無名科技Nologo-星智-1.54TFT</a>
- <a href="https://www.seeedstudio.com/SenseCAP-Watcher-W1-A-p-5979.html" target="_blank" title="SenseCAP Watcher">SenseCAP Watcher</a>
- <a href="https://www.bilibili.com/video/BV1BHJtz6E2S/" target="_blank" title="ESP-HI 超低コストロボット犬">ESP-HI 超低コストロボット犬</a>

<div style="display: flex; justify-content: space-between;">
  <a href="docs/v1/lichuang-s3.jpg" target="_blank" title="立創・実戦派 ESP32-S3 開発ボード">
    <img src="docs/v1/lichuang-s3.jpg" width="240" />
  </a>
  <a href="docs/v1/espbox3.jpg" target="_blank" title="楽鑫 ESP32-S3-BOX3">
    <img src="docs/v1/espbox3.jpg" width="240" />
  </a>
  <a href="docs/v1/m5cores3.jpg" target="_blank" title="M5Stack CoreS3">
    <img src="docs/v1/m5cores3.jpg" width="240" />
  </a>
  <a href="docs/v1/atoms3r.jpg" target="_blank" title="AtomS3R + Echo Base">
    <img src="docs/v1/atoms3r.jpg" width="240" />
  </a>
  <a href="docs/v1/magiclick.jpg" target="_blank" title="マジックボタン2.4">
    <img src="docs/v1/magiclick.jpg" width="240" />
  </a>
  <a href="docs/v1/waveshare.jpg" target="_blank" title="微雪電子 ESP32-S3-Touch-AMOLED-1.8">
    <img src="docs/v1/waveshare.jpg" width="240" />
  </a>
  <a href="docs/v1/lilygo-t-circle-s3.jpg" target="_blank" title="LILYGO T-Circle-S3">
    <img src="docs/v1/lilygo-t-circle-s3.jpg" width="240" />
  </a>
  <a href="docs/v1/xmini-c3.jpg" target="_blank" title="エビ兄さん Mini C3">
    <img src="docs/v1/xmini-c3.jpg" width="240" />
  </a>
  <a href="docs/v1/movecall-cuican-esp32s3.jpg" target="_blank" title="CuiCan">
    <img src="docs/v1/movecall-cuican-esp32s3.jpg" width="240" />
  </a>
  <a href="docs/v1/wmnologo_xingzhi_1.54.jpg" target="_blank" title="無名科技Nologo-星智-1.54">
    <img src="docs/v1/wmnologo_xingzhi_1.54.jpg" width="240" />
  </a>
  <a href="docs/v1/sensecap_watcher.jpg" target="_blank" title="SenseCAP Watcher">
    <img src="docs/v1/sensecap_watcher.jpg" width="240" />
  </a>
  <a href="docs/v1/esp-hi.jpg" target="_blank" title="ESP-HI 超低コストロボット犬">
    <img src="docs/v1/esp-hi.jpg" width="240" />
  </a>
</div>

## ソフトウェア

### ファームウェア書き込み

初心者の方は、まず開発環境を構築せずに書き込み可能なファームウェアを使用することをおすすめします。

ファームウェアはデフォルトで公式 [xiaozhi.me](https://xiaozhi.me) サーバーに接続します。個人ユーザーはアカウント登録でQwenリアルタイムモデルを無料で利用できます。

👉 [初心者向けファームウェア書き込みガイド](https://ccnphfhqs21z.feishu.cn/wiki/Zpz4wXBtdimBrLk25WdcXzxcnNS)

### 開発環境

- Cursor または VSCode
- ESP-IDFプラグインをインストールし、SDKバージョン5.4以上を選択
- LinuxはWindowsよりも優れており、コンパイルが速く、ドライバの問題も少ない
- 本プロジェクトはGoogle C++コードスタイルを採用、コード提出時は準拠を確認してください

### 開発者ドキュメント

- [カスタム開発ボードガイド](docs/custom-board.md) - シャオジーAI用のカスタム開発ボード作成方法
- [MCPプロトコルIoT制御使用法](docs/mcp-usage.md) - MCPプロトコルでIoTデバイスを制御する方法
- [MCPプロトコルインタラクションフロー](docs/mcp-protocol.md) - デバイス側MCPプロトコルの実装方法
- [MQTT + UDP ハイブリッド通信プロトコルドキュメント](docs/mqtt-udp.md)
- [詳細なWebSocket通信プロトコルドキュメント](docs/websocket.md)

## MCP と診断

MCP メッセージは、プロジェクトの WebSocket または MQTT トランスポート上を `type: "mcp"` メッセージとして、JSON-RPC 2.0 ペイロードで運ばれます。デバイスは MCP サーバーとして動作します。バックエンドはセッションを初期化し、`tools/list` でツールを列挙し、`tools/call` でデバイス機能を呼び出します。

本フォークは MCP `initialize` レスポンスにより豊富な機能メタデータを追加し、ユーザー専用（user-only）の診断ツールを公開します：

- `self.get_system_info`
- `self.diagnostics.get_checks`
- `self.diagnostics.run_check`
- `self.reboot`
- `self.upgrade_firmware`
- LVGL ディスプレイボードのスクリーンショット／プレビューツール
- カメラボードの撮影ツール

モデルが呼び出せる操作には通常ツールを、コンパニオンアプリや明示的なユーザー操作には user-only ツールを使います。[docs/mcp-usage.md](docs/mcp-usage.md) と [docs/mcp-protocol.md](docs/mcp-protocol.md) を参照してください。

### ボード向け MCP ツールの作り方

「MCP 機能」とは、アシスタント（またはコンパニオンアプリ）に呼び出し可能な関数として公開する任意のボード機能のことです——音量設定、ミュート切り替え、LED の色設定、スタンバイ移行、センサー読み取りなど。これは C 言語ライブラリではなく **C++** です。フレームワークは [`main/mcp_server.h`](main/mcp_server.h) にある少数のヘッダークラス群（`McpServer`、`McpTool`、`Property`、`PropertyList`）です。cJSON（C 言語ライブラリ）は JSON-RPC ペイロードのシリアライズに内部的に使われるだけで、通常のツール開発では一切触れません。

#### MCP ツール vs. 物理ボタン

これらは別のレイヤーであり、混同しやすいものです：

- **物理ボタン**（スタンバイ、ミュート、boot）はボードの `.cc` 内で `Button` クラスを介して配線され、通常は `ToggleChatState()` のような `Application` API を呼びます。人が押したときだけ動作します。
- **MCP ツール**は*同じ機能*をネットワーク経由で LLM やコンパニオンアプリに公開するため、モデルがそれを実行できます（「ミュートして」「スタンバイにして」）。

ミュートやスタンバイのような機能では、1 つのハンドラーメソッドを書き、ボタンのコールバックと MCP ツールの**両方**をそこに繋ぐのが良いパターンです。

#### 候補機能の見つけ方

ボード上の状態を持つもの、制御できるものはすべて候補です。よく現れる形は **getter + setter** のペア（現在の状態を読む、それを変更する）で、`Settings` ヘルパーで設定を永続化して再起動後も保持します。コピーできる既存のツリー内サンプル：

| 機能 | 場所 | ツール名 |
|------|------|----------|
| 長押しトーク vs クリックトークのモード | [`main/boards/common/press_to_talk_mcp_tool.cc`](main/boards/common/press_to_talk_mcp_tool.cc) | `self.set_press_to_talk` |
| RGB LED ストリップ | [`main/boards/df-k10/led_control.cc`](main/boards/df-k10/led_control.cc) | `self.led_strip.get_brightness`、`self.led_strip.set_brightness`、`self.led_strip.set_all_color` など |
| 音量 / 画面 / カメラ | [`main/mcp_server.cc`](main/mcp_server.cc) の `AddCommonTools()` | `self.audio_speaker.set_volume`、`self.screen.set_brightness`、`self.camera.take_photo` |

#### ツールの登録場所

- すべてのボードで共有される**共通ツール**（音量、明るさ、テーマ、カメラ、デバイス状態）は `main/mcp_server.cc` の `McpServer::AddCommonTools()` にあります。ここにボード固有のツールを**追加しないでください**。
- **ボード固有のツール**は、ボードクラスの `InitializeTools()` メソッドに置き、ボードのコンストラクタから呼びます。`Application` は起動時に一度 `AddCommonTools()` / `AddUserOnlyTools()` を呼び（`main/application.cc` 参照）、共通ツールをボードのツールの前に挿入します。

#### ツールの構造

```cpp
McpServer::GetInstance().AddTool(
    "self.audio_speaker.set_mute",          // 名前：self.<ドメイン>.<アクション>
    "Mute or unmute the speaker.",          // 説明。モデルはこれを読んで呼び出すか判断する
    PropertyList({                          // 入力スキーマ（空の PropertyList() = 引数なし）
        Property("mute", kPropertyTypeBoolean)
    }),
    [this](const PropertyList& properties) -> ReturnValue {   // コールバック
        bool mute = properties["mute"].value<bool>();
        Board::GetInstance().GetAudioCodec()->EnableOutput(!mute);
        return true;                        // ReturnValue: bool | int | std::string | cJSON* | ImageContent*
    });
```

プロパティの型とルール（`mcp_server.h` より）：

- `kPropertyTypeBoolean`、`kPropertyTypeInteger`、`kPropertyTypeString`。
- **デフォルト値のないプロパティは必須**です。デフォルト値付きで構築したものは任意です。
- 整数は範囲を取れます：`Property("level", kPropertyTypeInteger, 0, 8)`（最小/最大）または `Property("level", kPropertyTypeInteger, 4, 0, 8)`（デフォルト、最小、最大）。範囲外の値は例外を投げ、MCP エラーとして返されます。
- コールバック内では `properties["name"].value<T>()` で引数を読みます。

#### 通常ツール vs. user-only ツール

- `AddTool(...)`——LLM から見える。アシスタントが自分で実行してよい操作に使います。
- `AddUserOnlyTool(...)`——`audience: ["user"]` が注釈され、モデルからは隠され、コンパニオンアプリ／明示的なユーザー操作からのみ呼べます。`self.reboot`、`self.upgrade_firmware`、診断などの慎重に扱うべき操作に使います。

#### 実例：エンドツーエンドの「スタンバイ」機能

本プロジェクトにはすでに第一級のアイドル/スリープ機構があります——`PowerSaveTimer`（[`main/boards/common/power_save_timer.h`](main/boards/common/power_save_timer.h)）。以下の例はスタンバイを 3 通り——物理ボタン、MCP ツール、自動アイドルタイムアウト——で接続し、すべてが**同一**のハンドラーを共有するため、LLM、コンパニオンアプリ、人のいずれも同じ振る舞いを起こせます。

```cpp
#include "wifi_board.h"
#include "audio/audio_codec.h"
#include "boards/common/button.h"
#include "boards/common/power_save_timer.h"
#include "mcp_server.h"
#include "config.h"
#include <esp_log.h>

#define TAG "MyBoard"

class MyBoard : public WifiBoard {
private:
    Button standby_button_{STANDBY_BUTTON_GPIO};   // ボードの config.h から
    PowerSaveTimer* power_save_timer_ = nullptr;
    bool in_standby_ = false;

    // ---- ボタン + MCP ツール + アイドルタイマーで共有する単一ハンドラー ----
    void EnterStandby() {
        if (in_standby_) return;
        in_standby_ = true;
        ESP_LOGI(TAG, "Entering standby");
        GetAudioCodec()->EnableOutput(false);          // スピーカーを消音
        if (auto* bl = GetBacklight()) {
            bl->SetBrightness(0, true);                 // 画面オフ
        }
    }

    void ExitStandby() {
        if (!in_standby_) return;
        in_standby_ = false;
        ESP_LOGI(TAG, "Leaving standby");
        GetAudioCodec()->EnableOutput(true);
        if (auto* bl = GetBacklight()) {
            bl->RestoreBrightness();                    // ユーザーの明るさに戻す
        }
    }

    // ---- 1. アイドル後の自動スタンバイ ----
    void InitializePowerSaveTimer() {
        // cpu_max_freq, seconds_to_sleep, seconds_to_shutdown(-1 = しない)
        power_save_timer_ = new PowerSaveTimer(-1, 60);
        power_save_timer_->OnEnterSleepMode([this]() { EnterStandby(); });
        power_save_timer_->OnExitSleepMode([this]() { ExitStandby(); });
        power_save_timer_->SetEnabled(true);
    }

    // ---- 2. 物理ボタン：長押しでスタンバイ、クリックで復帰 ----
    void InitializeButtons() {
        standby_button_.OnLongPress([this]() { EnterStandby(); });
        standby_button_.OnClick([this]() {
            power_save_timer_->WakeUp();   // アイドルタイマーをリセットし OnExitSleepMode を発火
        });
    }

    // ---- 3. MCP ツール：アシスタント / コンパニオンアプリに実行させる ----
    void InitializeTools() {
        auto& mcp = McpServer::GetInstance();

        mcp.AddTool("self.system.enter_standby",
            "Put the device into low-power standby. The screen and speaker turn off. "
            "The device wakes on a button press.",
            PropertyList(),                            // 引数なし
            [this](const PropertyList&) -> ReturnValue {
                EnterStandby();
                return true;
            });

        // モデルが「スタンバイ中？」に答えられるよう getter を用意
        mcp.AddTool("self.system.get_standby",
            "Returns true if the device is currently in standby.",
            PropertyList(),
            [this](const PropertyList&) -> ReturnValue {
                return in_standby_;                    // ReturnValue は bool を受け取れる
            });
    }

public:
    MyBoard() {
        InitializePowerSaveTimer();
        InitializeButtons();
        InitializeTools();      // ツールはコンストラクタ中に登録する必要がある
    }

    virtual AudioCodec* GetAudioCodec() override { /* ...あなたの codec... */ }
    virtual Display* GetDisplay() override { /* ...あなたの display... */ }
};

DECLARE_BOARD(MyBoard);
```

各部分のつながり：

- **`PowerSaveTimer(cpu_max_freq, seconds_to_sleep, seconds_to_shutdown)`** は、`seconds_to_sleep` 秒の無活動後に `OnEnterSleepMode` を、復帰時に `OnExitSleepMode` を発火します。`WakeUp()` はカウントダウンをリセットしてスリープを抜けます——ユーザー操作のたびに呼んでください。任意の `seconds_to_shutdown` は完全電源オフ経路のために `OnShutdownRequest` を駆動します（ここでは `-1`/しない）。
- **ボタンとツールはどちらも同じ `EnterStandby()` を呼ぶ**——これが要のパターンです。ボタンはオフラインで動作し、MCP ツールは LLM（「スタンバイにして」）やコンパニオンアプリがネットワーク経由で同じ振る舞いを起こせます。
- **`EnableOutput(false)` + `SetBrightness(0)`** は、ボードがスピーカーと画面を静かにする実際のツリー内のやり方です。真のディープスリープが必要なら、`esp-spot`（[`main/boards/esp-spot/esp_spot_board.cc`](main/boards/esp-spot/esp_spot_board.cc)）は `EnterDeepSleep()` 内で GPIO/IMU ウェイクソース付きの `esp_deep_sleep` まで踏み込みます——「画面 + 音声オフ」で足りない場合はそのモデルに倣ってください。

`STANDBY_BUTTON_GPIO` はボードの `config.h` のピンに、`GetAudioCodec()`/`GetDisplay()`/`GetBacklight()` のオーバーライドはあなたのハードウェアに合わせてください。

#### テスト

ボードをビルドして書き込み、バックエンドからその WebSocket/MQTT トランスポート経由でデバイスを駆動します——サーバーが `tools/list`（あなたのツールが現れるはず）と `tools/call` を送ります。JSON-RPC のフレーミングと正確な `initialize` / `tools/list` / `tools/call` フローは [docs/mcp-protocol.md](docs/mcp-protocol.md) に記載されています。カメラツール専用にはローカルハーネス `scripts/local_camera_mcp_harness.py` があります。

## 大規模モデル設定

すでにシャオジーAIチャットボットデバイスをお持ちで、公式サーバーに接続済みの場合は、[xiaozhi.me](https://xiaozhi.me) コンソールで設定できます。

👉 [バックエンド操作ビデオチュートリアル（旧インターフェース）](https://www.bilibili.com/video/BV1jUCUY2EKM/)

## 関連オープンソースプロジェクト

個人PCでサーバーをデプロイする場合は、以下のオープンソースプロジェクトを参照してください：

- [xinnan-tech/xiaozhi-esp32-server](https://github.com/xinnan-tech/xiaozhi-esp32-server) Pythonサーバー
- [joey-zhou/xiaozhi-esp32-server-java](https://github.com/joey-zhou/xiaozhi-esp32-server-java) Javaサーバー
- [AnimeAIChat/xiaozhi-server-go](https://github.com/AnimeAIChat/xiaozhi-server-go) Golangサーバー
- [hackers365/xiaozhi-esp32-server-golang](https://github.com/hackers365/xiaozhi-esp32-server-golang) Golangサーバー

シャオジー通信プロトコルを利用した他のクライアントプロジェクト：

- [huangjunsen0406/py-xiaozhi](https://github.com/huangjunsen0406/py-xiaozhi) Pythonクライアント
- [TOM88812/xiaozhi-android-client](https://github.com/TOM88812/xiaozhi-android-client) Androidクライアント
- [100askTeam/xiaozhi-linux](http://github.com/100askTeam/xiaozhi-linux) 百問科技提供のLinuxクライアント
- [78/xiaozhi-sf32](https://github.com/78/xiaozhi-sf32) 思澈科技のBluetoothチップファームウェア
- [QuecPython/solution-xiaozhiAI](https://github.com/QuecPython/solution-xiaozhiAI) 移遠提供のQuecPythonファームウェア

## プロジェクトについて

これはエビ兄さんがオープンソースで公開しているESP32プロジェクトで、MITライセンスのもと、誰でも無料で、商用利用も可能です。

このプロジェクトを通じて、AIハードウェア開発を理解し、急速に進化する大規模言語モデルを実際のハードウェアデバイスに応用できるようになることを目指しています。

ご意見やご提案があれば、いつでもIssueを提出するか、[Discord](https://discord.gg/C759fGMBcZ) または QQグループ：1011329060 にご参加ください。

## スター履歴

<a href="https://star-history.com/#78/xiaozhi-esp32&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date" />
 </picture>
</a> 
