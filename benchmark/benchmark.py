from dataclasses import dataclass
import enum
import logging
import os
import subprocess
import time
from typing import List


class DataType(enum.Enum):
    F32 = 1
    F64 = 2
    U32 = 3
    U64 = 4


@dataclass
class Dataset:
    name: str
    path: str
    typ: DataType
    dim: int


DATA_DIR = "../data"

DATASETS = [
    Dataset(
        name="rsim.f32", path=f"{DATA_DIR}/fabian/rsim.f32", typ=DataType.F32, dim=1
    ),
    Dataset(
        name="rsim.f64", path=f"{DATA_DIR}/fabian/rsim.f64", typ=DataType.F64, dim=1
    ),
    Dataset(
        name="astro_mhd.f32",
        path=f"{DATA_DIR}/fabian/astro_mhd.f32",
        typ=DataType.F32,
        dim=1,
    ),
    Dataset(
        name="astro_mhd.f32",
        path=f"{DATA_DIR}/fabian/astro_mhd.f64",
        typ=DataType.F64,
        dim=1,
    ),
    Dataset(
        name="astro_pt.f32",
        path=f"{DATA_DIR}/fabian/astro_pt.f32",
        typ=DataType.F32,
        dim=3,
    ),
    Dataset(
        name="astro_pt.f64",
        path=f"{DATA_DIR}/fabian/astro_pt.f64",
        typ=DataType.F64,
        dim=3,
    ),
    Dataset(
        name="wave.f32", path=f"{DATA_DIR}/fabian/wave.f32", typ=DataType.F32, dim=2
    ),
    Dataset(
        name="wave.f64", path=f"{DATA_DIR}/fabian/wave.f64", typ=DataType.F64, dim=2
    ),
]


@dataclass
class Result:
    compressor: str
    args: List[str]
    ds: Dataset
    raw_size: int
    compressed_size: int
    duration: float


def check_call(args: List[str]) -> None:
    logging.info(f"run: {args}")
    subprocess.check_call(args)


def native_zstd(ds: Dataset, level: int) -> Result:
    """Run the zstd command."""
    start = time.time()
    check_call(["zstd", f"-{level}", "-o", "/tmp/testout", "-f", ds.path])
    duration = time.time() - start
    return Result(
        compressor="zstd",
        args=[f"{level}"],
        ds=ds,
        raw_size=os.path.getsize(ds.path),
        compressed_size=os.path.getsize("/tmp/testout"),
        duration=duration,
    )

def main() -> None:
    logging.basicConfig(
        format="%(asctime)s,%(msecs)d %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s",
        datefmt="%Y-%m-%d:%H:%M:%S",
        level=logging.INFO,
    )
    print("XXX")
    print(native_zstd(DATASETS[0], 3))
    print(native_zstd(DATASETS[0], 20))

main()
# https://computing.llnl.gov/projects/fpzip
