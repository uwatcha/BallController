# Author: Claude
# Content フォルダの整理スクリプト（2026-11-23、スレッド『Contentフォルダを整理する』）
# UE エディタの「ツール」>「Python スクリプトを実行...」から実行する。
# 1. 不要アセットを削除（外からの参照があれば削除せずにスキップ）
# 2. アセットを種類ごとのフォルダへ移動
# 3. リダイレクタを修正して保存
# 4. DefaultEngine.ini のゲームモードのパスを書き換え
import os
import unreal

EAL = unreal.EditorAssetLibrary

# ---- 削除するもの（フォルダは末尾 / なし、どちらも /Game から）----
DELETE_DIRS = [
    "/Game/Characters/Mannequins",
    "/Game/Input/Touch",
    "/Game/LevelPrototyping/Interactable/Target",
]
DELETE_ASSETS = [
    "/Game/Characters/SteelBall/Materials/M_Steel",
    "/Game/Input/IMC_Default",
    "/Game/Input/IMC_MouseLook",
    "/Game/Input/IMC_BallKeyController",
    "/Game/Input/IA_BallKeyController",
    "/Game/Input/Actions/IA_Move",
    "/Game/Input/Actions/IA_Jump",
    "/Game/Input/Actions/IA_Look",
    "/Game/Input/Actions/IA_MouseLook",
]

# ---- 移動するもの ----
MOVES = [
    ("/Game/ThirdPerson/Blueprints_SteelBall/BP_SteelBallGameMode", "/Game/Blueprints/Core/BP_SteelBallGameMode"),
    ("/Game/ThirdPerson/Blueprints_SteelBall/BP_SteelBallPawn", "/Game/Blueprints/Core/BP_SteelBallPawn"),
    ("/Game/ThirdPerson/Blueprints_Cursor/BP_CursorController", "/Game/Blueprints/Core/BP_CursorController"),
    ("/Game/Setting/MyPrimaryDataAsset", "/Game/Data/MyPrimaryDataAsset"),
    ("/Game/ThirdPerson/MI_ThirdPersonColWay", "/Game/Materials/MI_ThirdPersonColWay"),
]
NEW_DIRS = ["/Game/Blueprints/Core", "/Game/Blueprints/Gimmicks", "/Game/Data", "/Game/Materials", "/Game/Maps"]

# 空になったら消すフォルダ（深い順）
CLEANUP_DIRS = [
    "/Game/ThirdPerson/Blueprints_SteelBall",
    "/Game/ThirdPerson/Blueprints_Cursor",
    "/Game/Setting",
    "/Game/Characters/SteelBall/Materials",
    "/Game/Characters/SteelBall",
    "/Game/Characters",
]


def pkg(path):
    """'/Game/A/B.B' や '/Game/A/B' をパッケージ名 '/Game/A/B' にする"""
    return path.split(".")[0]


def log(msg):
    unreal.log("[Reorganize] " + msg)


def warn(msg):
    unreal.log_warning("[Reorganize] " + msg)


def collect_delete_set():
    targets = {}  # 対象名 -> その中のパッケージ一覧
    for d in DELETE_DIRS:
        if EAL.does_directory_exist(d):
            targets[d] = [pkg(a) for a in EAL.list_assets(d, recursive=True, include_folder=False)]
        else:
            warn("フォルダがありません（スキップ）: " + d)
    for a in DELETE_ASSETS:
        if EAL.does_asset_exist(a):
            targets[a] = [a]
        else:
            warn("アセットがありません（スキップ）: " + a)
    return targets


def step_delete():
    targets = collect_delete_set()
    all_pkgs = set(p for ps in targets.values() for p in ps)
    for name, pkgs in targets.items():
        outside = set()
        for p in pkgs:
            for r in EAL.find_package_referencers_for_asset(p, load_assets_to_confirm=False):
                if pkg(str(r)) not in all_pkgs:
                    outside.add(str(r))
        if outside:
            warn("外から参照されているので削除しません: %s <- %s" % (name, ", ".join(sorted(outside))))
            continue
        ok = EAL.delete_directory(name) if name in DELETE_DIRS else EAL.delete_asset(name)
        log(("削除しました: " if ok else "削除に失敗: ") + name)


def step_move():
    for d in NEW_DIRS:
        EAL.make_directory(d)
    for src, dst in MOVES:
        if not EAL.does_asset_exist(src):
            warn("移動元がありません（スキップ）: " + src)
            continue
        if EAL.does_asset_exist(dst):
            warn("移動先にもうあります（スキップ）: " + dst)
            continue
        ok = EAL.rename_asset(src, dst)
        log(("移動しました: " if ok else "移動に失敗: ") + src + " -> " + dst)


def step_fix_redirectors():
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    flt = unreal.ARFilter(
        class_paths=[unreal.TopLevelAssetPath("/Script/CoreUObject", "ObjectRedirector")],
        package_paths=["/Game"],
        recursive_paths=True,
    )
    reds = [a.get_asset() for a in ar.get_assets(flt)]
    reds = [r for r in reds if r]
    if reds:
        unreal.AssetToolsHelpers.get_asset_tools().fix_up_referencers(reds)
    log("リダイレクタを修正しました: %d 個" % len(reds))


def step_cleanup_dirs():
    for d in CLEANUP_DIRS:
        if EAL.does_directory_exist(d) and not EAL.does_directory_have_assets(d, recursive=True):
            EAL.delete_directory(d)
            log("空のフォルダを削除しました: " + d)


def step_fix_config():
    ini = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_config_dir()), "DefaultEngine.ini")
    old = "/Game/ThirdPerson/Blueprints_SteelBall/BP_SteelBallGameMode.BP_SteelBallGameMode_C"
    new = "/Game/Blueprints/Core/BP_SteelBallGameMode.BP_SteelBallGameMode_C"
    with open(ini, encoding="utf-8") as f:
        text = f.read()
    if old in text:
        with open(ini, "w", encoding="utf-8") as f:
            f.write(text.replace(old, new))
        log("DefaultEngine.ini のゲームモードのパスを書き換えました")


def main():
    log("開始")
    step_delete()
    step_move()
    step_fix_redirectors()
    EAL.save_directory("/Game", only_if_is_dirty=True, recursive=True)
    step_cleanup_dirs()
    step_fix_config()
    log("完了。エディタを再起動してください")


main()
