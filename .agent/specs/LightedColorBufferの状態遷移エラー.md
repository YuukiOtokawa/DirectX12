# LightedColorBuffer の状態遷移エラー

- 状態: 提案（原因候補の静的調査済み、修正は未実施）
- 更新日: 2026-10-28
- 動作検証: ユーザーから実行時エラーの報告あり。AIによる再現・修正後検証は未実施
- 関連資料: [プロジェクト分割](プロジェクト分割.md)

## 目的と対象範囲

`LightedColorBuffer` のリソース状態と、描画・ResourceBarrier が前提とする状態の不一致を解消する。

プロジェクト分割 Phase 5（ディレクトリ再配置）とは別件として扱う。今回保存するのは調査結果と引き継ぎであり、描画コードは修正していない。

## 報告された現象

ユーザーによると見た目の挙動は問題なさそうだが、実行中に次のエラーが出る。

```text
D3D12 ERROR: ID3D12CommandQueue1::ExecuteCommandLists:
Using ResourceBarrier on Command List (...:'GraphicsCommandList'):
Before state (0x4: D3D12_RESOURCE_STATE_RENDER_TARGET)
of resource (...:'LightedColorBuffer') (subresource: 0)
specified by transition barrier does not match with the state
(0x80: D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
specified in preceding ResourceBarrier or as InitialState
[ RESOURCE_MANIPULATION ERROR #527: RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH]
```

アドレスは実行ごとに変わるため省略した。報告時の構成（Debug / Release）、exe（Editor / Player）、発生頻度、直前の操作は未確認。

これは見た目だけでは判定できない状態遷移の不整合である。Microsoft の仕様でも、遷移の Before 状態と直前の After 状態が一致することを検証するとされている。

- 参考: [Microsoft DirectX specifications — Runtime Validation](https://microsoft.github.io/DirectX-Specs/d3d/CPUEfficiency.html)

## 期待する挙動

- 初回描画、通常フレーム、リサイズ後の描画で、実際のリソース状態と ResourceBarrier の StateBefore が一致する。
- ライティング・Forward 描画前には書き込み可能な状態、シェーダーで読む前には読み取り可能な状態にする。
- Game View / Scene View の連続描画、Bloom・ポストプロセスの有無でも整合性を保つ。
- D3D12 デバッグレイヤーで対象エラーが出ず、従来の描画を維持する。

## 公開API・編集項目

未決定。ユーザー向け設定追加は現時点では想定していない。

修正案として、生成時の状態を個別指定する方法、描画前後の状態を統一する方法、現在状態を追跡する方法が考えられる。共通 Factory の初期状態を一律に変えると他の描画ターゲットにも影響するため、LightedColorBuffer だけを見て変更しない。

## 処理の流れと所有権

- `RenderTargetManager` が `m_LightedColorBuffer` を所有する。
- `InitGBuffers()` で生成し、`ApplyPendingResizes()` のスワップチェーンリサイズ経路で再生成する。
- 生成処理は `RenderTargetFactory::Create()` に委譲する。
- `RenderManager` が描画・状態遷移を記録する。`RenderSystem::Render()` からライティング、Forward、Bloom、ポストプロセスが呼ばれ、`DrawEnd()` が表示用ターゲットへコピーする。
- 修正時は GPU 完了待ち・再生成の既存手順を保持し、再生成されたリソースにも正しい初期状態を適用する。

## 現在の実装と差異

以下のパスはリポジトリ直下からの相対パス。行番号は調査時点の目安で、再開時はシンボルを検索する。

| 箇所 | 静的に確認した内容 |
| --- | --- |
| `Engine/Source/Render/RenderTargetFactory.cpp:52` の `Create()` | `CreateCommittedResource()` の初期状態は `PIXEL_SHADER_RESOURCE`（55行目） |
| `Engine/Source/Render/RenderTargetManager.cpp:89,305` | LightedColorBuffer の初回生成・リサイズ時の再生成で、この Factory を使用する |
| `Engine/Source/Render/RenderManager.cpp:682` の `ResolveDeferredLighting()` | LightedColorBuffer がすでに `RENDER_TARGET` である前提で、RTV設定・クリア・描画を行う。ここで初期状態からの遷移は記録しない |
| 同ファイル `ApplyBloom()`（936行目）、`ApplyPostProcess()`（724行目） | 入口で LightedColorBuffer が `RENDER_TARGET` である前提を持つ |
| 同ファイル `DrawEnd()`（551行目） | 表示用コピー前に `RENDER_TARGET → PIXEL_SHADER_RESOURCE`、コピー後に逆方向の遷移を記録する |

### 原因候補

生成時は `PIXEL_SHADER_RESOURCE` なのに、使用側は `RENDER_TARGET` を前提としている。生成直後やリサイズによる再生成後、この前提の食い違いが報告されたエラーを起こす経路がある。

ただし、報告された実行で最初に不一致を起こした ResourceBarrier の呼び出し箇所までは特定していない。初期状態の不整合だけが原因とは断定せず、実行時の記録順序も確認する。

### Phase 5 との関係

Phase 5 前の `219e227` の `Code/Render/RenderManager.cpp` と現在のファイルは、include パスの変更と末尾空白の正規化を除いて内容が一致した。Factory と RenderTargetManager も include パス以外は一致した。

ローカル `master` にも、Factory の初期状態が `PIXEL_SHADER_RESOURCE` であることと、ライティング処理が `RENDER_TARGET` を前提とする記述が存在することを確認した。したがって、この不整合は今回のディレクトリ再配置で導入したものではない。ただし、旧版を実行して同じログを再現したわけではない。

## 受け入れ条件と検証

### 次回の調査・修正手順

1. エラーが出た exe、構成、発生タイミングを確認する。
2. D3D12 デバッグレイヤーで初回描画とリサイズ後を確認し、LightedColorBuffer の生成から最初の不一致までの状態遷移を追う。
3. 初期状態と各パスの入口・出口の状態を整理して、修正方法を決める。
4. シャドウパス中の呼び出し、カメラ不在・オブジェクト不在の経路、Graphics Debug Window による SRV 参照も確認する。これらは追加の調査対象であり、今回のエラー原因として確定したものではない。
5. 修正後、以下の条件で確認する。

| 検証 | 受け入れ条件 |
| --- | --- |
| 静的確認 | 初回生成・再生成・各パスで状態遷移が連続し、StateBefore の前提が一致する |
| C++ ビルド | Debug / Release x64 の Editor・Player がビルドできる |
| 実行時検証 | 初回描画、複数フレーム、ウィンドウリサイズ後に #527 が出ない。関連する状態エラーも確認する |
| 分岐の確認 | Bloom 有効／無効、ポストプロセスなし／あり、Editor の複数 View と Player の経路を確認する |
| 画面確認 | Game View / Scene View / Player の描画が維持される |

今回実施したのは静的調査のみ。Phase 5 のビルド成功は、この実行時エラーの解消を保証しない。

## 未決定事項

- 修正方式と変更範囲。
- エラーの発生頻度と正確な再現条件。
- 初期状態以外にも不整合を起こす経路があるか。
- 修正後の実行・画面確認の担当と結果。
