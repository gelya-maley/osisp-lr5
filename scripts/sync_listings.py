import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DST = ROOT / "report" / "listings"

FILES = {
    ROOT / "src" / "server" / "server.cpp": DST / "server.cpp",
    ROOT / "src" / "client" / "client.cpp": DST / "client.cpp",
    ROOT / "scripts" / "plot_rtt.py": DST / "plot_rtt.py",
    ROOT / "scripts" / "plot_errors.py": DST / "plot_errors.py",
    ROOT / "diagrams" / "sequence.puml": DST / "sequence.puml",
}


def main():
    DST.mkdir(parents=True, exist_ok=True)
    for src, dst in FILES.items():
        shutil.copyfile(src, dst)
        print("synced", dst.name)


if __name__ == "__main__":
    main()
