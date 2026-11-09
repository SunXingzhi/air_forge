#!/usr/bin/env python3
"""AirForge 固件构建入口。

为什么需要这个脚本：
    ESP-IDF v6.1 在 Windows 上由 EIM 安装，`idf.py` 只有在 PowerShell
    profile 被 source 之后才能正确找到 Python venv、xtensa 工具链和 ninja。
    直接双击或在普通终端执行 `idf.py build` 常报
    "No module named 'rich_click'" 或 "compiler not found"。

本脚本做四件事：
    1. 优先从 EIM profile 的 `-e` 输出恢复完整环境变量（不改全局环境）；
    2. 用该环境里的 venv Python 调用 `idf.py`，避免 PATH/venv 错配；
    3. 构建指定 build 目录的固件，可选 clean / flash / monitor；
    4. 把固件与仿真的 compile_commands.json 合并到根目录，供 clangd 使用。

用法：
    python scripts/build_app.py                     # 增量构建 build/
    python scripts/build_app.py --clean             # fullclean 后重建
    python scripts/build_app.py --build-dir build_x # 指定构建目录
    python scripts/build_app.py --flash --port COM9 # 构建后烧录
    python scripts/build_app.py --monitor           # 构建后打开串口监视
    python scripts/build_app.py --no-cdb            # 不合并 compile_commands
"""

import argparse
import glob
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BUILD_DIR = ROOT / "build"
SIM_CDB = ROOT / "sim" / "build" / "compile_commands.json"

# EIM profile 会输出这些变量；只接收白名单，避免污染子进程环境
ENV_KEYS = {
    "IDF_PATH",
    "IDF_TOOLS_PATH",
    "IDF_PYTHON_ENV_PATH",
    "IDF_COMPONENT_LOCAL_STORAGE_URL",
    "ESP_ROM_ELF_DIR",
    "OPENOCD_SCRIPTS",
    "ESP_CLANG_LIBS_PATH",
    "IDF_CCACHE_ENABLE",
    "ESP_IDF_VERSION",
    "IDF_VERSION",
    "PATH",
}


def log(message: str) -> None:
    """打印带统一前缀的进度信息；不写日志文件，失败信息直接留在终端。"""
    print(f"[build_app] {message}", flush=True)


def find_windows_profile() -> Path | None:
    """定位 EIM 生成的 PowerShell profile。

    搜索顺序：IDF_TOOLS_PATH 根目录、IDF_TOOLS_PATH/tools、~/.espressif。
    多个版本同时存在时取最近修改的一个，避免误用旧版工具链。
    """
    candidates: list[Path] = []
    roots = [
        os.environ.get("IDF_TOOLS_PATH"),
        str(Path.home() / ".espressif"),
    ]
    for root in roots:
        if not root:
            continue
        for base in (root, os.path.join(root, "tools")):
            candidates.extend(Path(p) for p in glob.glob(os.path.join(base, "Microsoft.*PowerShell_profile.ps1")))

    existing = [p for p in candidates if p.is_file()]
    if not existing:
        return None
    return max(existing, key=lambda p: p.stat().st_mtime)


def load_windows_env(profile: Path) -> dict[str, str]:
    """通过 profile 的 `-e` 模式读取 ESP-IDF 环境变量，不执行构建。

    profile 会把工具链路径拼进 PATH，并把 venv 路径写入
    IDF_PYTHON_ENV_PATH。`-e` 分支在设置环境后立即返回，不会触发
    终端补全注册，因此启动很快。
    """
    command = [
        "powershell",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(profile),
        "-e",
    ]
    result = subprocess.run(command, capture_output=True, text=True, timeout=60)
    if result.returncode != 0:
        raise RuntimeError(
            "读取 ESP-IDF 环境失败：\n"
            f"{result.stdout}\n{result.stderr}".strip()
        )

    env: dict[str, str] = {}
    for raw_line in result.stdout.splitlines():
        line = raw_line.strip()
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if key in ENV_KEYS:
            env[key] = value
    return env


def idf_python_and_script(env: dict[str, str]) -> tuple[Path, Path]:
    """从 ESP-IDF 环境解析 venv Python 与 idf.py 的绝对路径。

    不使用 PATH 中的 `idf.py`：Windows 下它可能是 .py 文件关联，
    在管道/子进程里会退化成“用默认程序打开”，而不是执行脚本。
    """
    idf_path = env.get("IDF_PATH")
    venv = env.get("IDF_PYTHON_ENV_PATH")
    if not idf_path or not venv:
        raise RuntimeError(
            "ESP-IDF 环境不完整：缺少 IDF_PATH 或 IDF_PYTHON_ENV_PATH。"
            "请先运行 ESP-IDF 安装器的 export 脚本。"
        )

    idf_script = Path(idf_path) / "tools" / "idf.py"
    if not idf_script.is_file():
        raise RuntimeError(f"找不到 idf.py：{idf_script}")

    candidates = [
        Path(venv) / "Scripts" / "python.exe",
        Path(venv) / "bin" / "python",
    ]
    for python in candidates:
        if python.is_file():
            return python, idf_script

    raise RuntimeError(f"ESP-IDF Python venv 不存在或缺少解释器：{venv}")


def build_env() -> dict[str, str]:
    """为 idf.py 子进程准备环境。

    Windows 优先走 EIM profile；若当前终端已经完整激活 ESP-IDF，
    则直接沿用现有环境。POSIX 环境交给调用方的 export.sh 处理。
    """
    env = dict(os.environ)

    if os.name == "nt":
        profile = find_windows_profile()
        if profile is not None:
            try:
                parsed = load_windows_env(profile)
                # profile 的 PATH 只包含 ESP-IDF 工具链；必须拼回系统 PATH，
                # 否则 git、系统 CMake 等外部工具会在构建中“消失”
                original_path = env.get("PATH", "")
                env.update(parsed)
                if parsed.get("PATH"):
                    env["PATH"] = parsed["PATH"] + os.pathsep + original_path
                log(f"ESP-IDF 环境：{profile.name}")
            except (OSError, RuntimeError, subprocess.TimeoutExpired) as exc:
                log(f"读取 profile 失败，尝试沿用当前环境：{exc}")

    if env.get("IDF_PATH") and env.get("IDF_PYTHON_ENV_PATH"):
        return env

    # 没有现成 profile 时，最后尝试 PATH 里的 idf.py；失败由调用方报错
    return env


def resolve_idf_command(idf_args: list[str], env: dict[str, str]) -> list[str]:
    """构造 idf.py 调用命令。

    Windows：显式用 venv Python 执行 tools/idf.py，彻底绕开 PATH 与
    文件关联问题。POSIX：若环境未激活，用 bash source export.sh 后再执行。
    """
    try:
        python, script = idf_python_and_script(env)
        return [str(python), str(script), *idf_args]
    except RuntimeError:
        if os.name == "nt":
            raise

    if shutil.which("idf.py") and env.get("IDF_PATH"):
        return ["idf.py", *idf_args]

    # POSIX 兜底：在子 shell 内 source export.sh，不修改当前进程环境
    cmd = (
        'source "$IDF_PATH/export.sh" >/dev/null 2>&1 || exit 1; '
        'exec idf.py "$@"'
    )
    return ["bash", "-c", cmd, "idf.py", *idf_args]


def run_idf(idf_args: list[str], env: dict[str, str]) -> int:
    """执行一次 idf.py 并返回退出码；子进程直接继承终端，可交互 monitor。"""
    command = resolve_idf_command(idf_args, env)
    log("执行：" + " ".join(command))
    result = subprocess.run(command, env=env, cwd=str(ROOT))
    return result.returncode


def merge_compile_commands(build_dir: Path) -> None:
    """把固件与仿真的编译数据库合并到根目录 compile_commands.json。

    clangd 只读取根目录这一份（`.vscode/settings.json`）。固件新增源文件后
    若不重新合并，跳转定义会退化为只到声明。按 (directory, file, command)
    三元组去重，避免重复条目撑大文件。
    """
    sources = [build_dir / "compile_commands.json", SIM_CDB]
    merged: list[dict] = []
    seen: set[tuple[str, str, str]] = set()

    for source in sources:
        if not source.is_file():
            log(f"跳过不存在的编译数据库：{source}")
            continue
        try:
            entries = json.loads(source.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            log(f"跳过无法解析的编译数据库 {source}：{exc}")
            continue

        added = 0
        for entry in entries:
            key = (
                str(entry.get("directory", "")),
                str(entry.get("file", "")),
                str(entry.get("command", entry.get("arguments", ""))),
            )
            if key in seen:
                continue
            seen.add(key)
            merged.append(entry)
            added += 1
        log(f"合并 {source.relative_to(ROOT)}：{added} 条")

    if not merged:
        log("没有可合并的编译数据库，保持根目录文件不变")
        return

    target = ROOT / "compile_commands.json"
    tmp = target.with_suffix(".json.tmp")
    tmp.write_text(
        json.dumps(merged, ensure_ascii=False, separators=(",", ":")),
        encoding="utf-8",
    )
    os.replace(tmp, target)
    log(f"已写入 {target}（{len(merged)} 条）")


def report_binary(build_dir: Path) -> None:
    """构建成功后打印固件大小，便于快速判断分区余量。"""
    binary = build_dir / "air_forge.bin"
    if binary.is_file():
        log(f"固件：{binary}（{binary.stat().st_size} bytes）")


def parse_args() -> argparse.Namespace:
    """解析命令行参数；默认行为就是 `idf.py build`，不改变现有构建目录。"""
    parser = argparse.ArgumentParser(
        description="构建 AirForge ESP32-S3 固件（自动引导 ESP-IDF 环境）"
    )
    parser.add_argument(
        "--build-dir",
        default=str(DEFAULT_BUILD_DIR),
        help="构建目录，默认 build/",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="先执行 idf.py fullclean，再重新构建",
    )
    parser.add_argument(
        "--target",
        default=None,
        help="可选：先执行 set-target（会清空 build 目录），如 esp32s3",
    )
    parser.add_argument(
        "--flash",
        action="store_true",
        help="构建后烧录",
    )
    parser.add_argument(
        "--monitor",
        action="store_true",
        help="构建后打开串口监视（可与 --flash 组合）",
    )
    parser.add_argument(
        "--port",
        default=None,
        help="串口，如 COM9；不填则由 idf.py 自动探测",
    )
    parser.add_argument(
        "--no-cdb",
        action="store_true",
        help="不合并 compile_commands.json",
    )
    return parser.parse_args()


def main() -> int:
    """主流程：环境 → (clean/set-target) → build → 烧录/监视 → 合并 CDB。"""
    args = parse_args()
    build_dir = Path(args.build_dir).resolve()

    env = build_env()
    if not env.get("IDF_PATH"):
        log("未找到 IDF_PATH。请先安装 ESP-IDF 并运行其 PowerShell profile。")
        return 2

    try:
        idf_python_and_script(env)
    except RuntimeError as exc:
        if os.name == "nt":
            log(str(exc))
            log("提示：在 ESP-IDF 安装目录运行 Microsoft.*.PowerShell_profile.ps1 可修复环境。")
            return 2
        # POSIX 允许没有 venv 的情况：run_idf 会退回 bash source export.sh
        log(f"未找到现成 venv，尝试通过 export.sh 激活：{exc}")

    log(f"工程目录：{ROOT}")
    log(f"构建目录：{build_dir}")

    if args.target:
        code = run_idf(["-B", str(build_dir), "set-target", args.target], env)
        if code != 0:
            return code

    if args.clean:
        code = run_idf(["-B", str(build_dir), "fullclean"], env)
        if code != 0:
            return code

    code = run_idf(["-B", str(build_dir), "build"], env)
    if code != 0:
        log(f"构建失败，退出码 {code}")
        return code
    report_binary(build_dir)

    if args.flash or args.monitor:
        action: list[str] = []
        if args.port:
            action.extend(["-p", args.port])
        if args.flash:
            action.append("flash")
        if args.monitor:
            action.append("monitor")
        code = run_idf(["-B", str(build_dir), *action], env)
        if code != 0:
            return code

    if not args.no_cdb:
        merge_compile_commands(build_dir)

    log("完成")
    return 0


if __name__ == "__main__":
    sys.exit(main())
