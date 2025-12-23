"""Analyze phase - parse debug.log and generate performance plots.

Refactored from bench-ci/parse_and_plot.py for better structure and reusability.
"""

from __future__ import annotations

import datetime
import logging
import re
from collections import OrderedDict
from dataclasses import dataclass
from pathlib import Path

# matplotlib is optional - gracefully handle if not installed
try:
    import matplotlib.pyplot as plt

    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

logger = logging.getLogger(__name__)

# Bitcoin fork heights for plot annotations
FORK_HEIGHTS = OrderedDict(
    [
        ("BIP34", 227931),  # Block v2, coinbase includes height
        ("BIP66", 363725),  # Strict DER signatures
        ("BIP65", 388381),  # OP_CHECKLOCKTIMEVERIFY
        ("CSV", 419328),  # BIP68, 112, 113 - OP_CHECKSEQUENCEVERIFY
        ("Segwit", 481824),  # BIP141, 143, 144, 145 - Segregated Witness
        ("Taproot", 709632),  # BIP341, 342 - Schnorr signatures & Taproot
        ("Halving 1", 210000),  # First halving
        ("Halving 2", 420000),  # Second halving
        ("Halving 3", 630000),  # Third halving
        ("Halving 4", 840000),  # Fourth halving
    ]
)

FORK_COLORS = {
    "BIP34": "blue",
    "BIP66": "blue",
    "BIP65": "blue",
    "CSV": "blue",
    "Segwit": "green",
    "Taproot": "red",
    "Halving 1": "purple",
    "Halving 2": "purple",
    "Halving 3": "purple",
    "Halving 4": "purple",
}

FORK_STYLES = {
    "BIP34": "--",
    "BIP66": "--",
    "BIP65": "--",
    "CSV": "--",
    "Segwit": "--",
    "Taproot": "--",
    "Halving 1": ":",
    "Halving 2": ":",
    "Halving 3": ":",
    "Halving 4": ":",
}


@dataclass
class UpdateTipEntry:
    """Parsed UpdateTip log entry."""

    timestamp: datetime.datetime
    height: int
    tx_count: int
    cache_size_mb: float
    cache_coins_count: int


@dataclass
class LevelDBCompactEntry:
    """Parsed LevelDB compaction log entry."""

    timestamp: datetime.datetime


@dataclass
class LevelDBGenTableEntry:
    """Parsed LevelDB generated table log entry."""

    timestamp: datetime.datetime
    keys_count: int
    bytes_count: int


@dataclass
class ValidationTxAddEntry:
    """Parsed validation transaction added log entry."""

    timestamp: datetime.datetime


@dataclass
class CoinDBWriteBatchEntry:
    """Parsed coindb write batch log entry."""

    timestamp: datetime.datetime
    is_partial: bool
    size_mb: float


@dataclass
class CoinDBCommitEntry:
    """Parsed coindb commit log entry."""

    timestamp: datetime.datetime
    txout_count: int


@dataclass
class ParsedLog:
    """All parsed data from a debug.log file."""

    update_tip: list[UpdateTipEntry]
    leveldb_compact: list[LevelDBCompactEntry]
    leveldb_gen_table: list[LevelDBGenTableEntry]
    validation_txadd: list[ValidationTxAddEntry]
    coindb_write_batch: list[CoinDBWriteBatchEntry]
    coindb_commit: list[CoinDBCommitEntry]


@dataclass
class AnalyzeResult:
    """Result of the analyze phase."""

    commit: str
    output_dir: Path
    plots: list[Path]


class LogParser:
    """Parse bitcoind debug.log files."""

    # Regex patterns
    UPDATETIP_RE = re.compile(
        r"^([\d\-:TZ]+) UpdateTip: new best.+height=(\d+).+tx=(\d+).+cache=([\d.]+)MiB\((\d+)txo\)"
    )
    LEVELDB_COMPACT_RE = re.compile(r"^([\d\-:TZ]+) \[leveldb] Compacting.*files")
    LEVELDB_GEN_TABLE_RE = re.compile(
        r"^([\d\-:TZ]+) \[leveldb] Generated table.*: (\d+) keys, (\d+) bytes"
    )
    VALIDATION_TXADD_RE = re.compile(
        r"^([\d\-:TZ]+) \[validation] TransactionAddedToMempool: txid=.+wtxid=.+"
    )
    COINDB_WRITE_BATCH_RE = re.compile(
        r"^([\d\-:TZ]+) \[coindb] Writing (partial|final) batch of ([\d.]+) MiB"
    )
    COINDB_COMMIT_RE = re.compile(
        r"^([\d\-:TZ]+) \[coindb] Committed (\d+) changed transaction outputs"
    )

    @staticmethod
    def parse_timestamp(iso_str: str) -> datetime.datetime:
        """Parse ISO 8601 timestamp from log."""
        return datetime.datetime.strptime(iso_str, "%Y-%m-%dT%H:%M:%SZ")

    def parse_file(self, log_file: Path) -> ParsedLog:
        """Parse a debug.log file and extract all relevant data."""
        update_tip: list[UpdateTipEntry] = []
        leveldb_compact: list[LevelDBCompactEntry] = []
        leveldb_gen_table: list[LevelDBGenTableEntry] = []
        validation_txadd: list[ValidationTxAddEntry] = []
        coindb_write_batch: list[CoinDBWriteBatchEntry] = []
        coindb_commit: list[CoinDBCommitEntry] = []

        with open(log_file, "r", encoding="utf-8") as f:
            for line in f:
                if match := self.UPDATETIP_RE.match(line):
                    iso_str, height, tx, cache_mb, coins = match.groups()
                    update_tip.append(
                        UpdateTipEntry(
                            timestamp=self.parse_timestamp(iso_str),
                            height=int(height),
                            tx_count=int(tx),
                            cache_size_mb=float(cache_mb),
                            cache_coins_count=int(coins),
                        )
                    )
                elif match := self.LEVELDB_COMPACT_RE.match(line):
                    leveldb_compact.append(
                        LevelDBCompactEntry(
                            timestamp=self.parse_timestamp(match.group(1))
                        )
                    )
                elif match := self.LEVELDB_GEN_TABLE_RE.match(line):
                    iso_str, keys, bytes_count = match.groups()
                    leveldb_gen_table.append(
                        LevelDBGenTableEntry(
                            timestamp=self.parse_timestamp(iso_str),
                            keys_count=int(keys),
                            bytes_count=int(bytes_count),
                        )
                    )
                elif match := self.VALIDATION_TXADD_RE.match(line):
                    validation_txadd.append(
                        ValidationTxAddEntry(
                            timestamp=self.parse_timestamp(match.group(1))
                        )
                    )
                elif match := self.COINDB_WRITE_BATCH_RE.match(line):
                    iso_str, batch_type, size_mb = match.groups()
                    coindb_write_batch.append(
                        CoinDBWriteBatchEntry(
                            timestamp=self.parse_timestamp(iso_str),
                            is_partial=(batch_type == "partial"),
                            size_mb=float(size_mb),
                        )
                    )
                elif match := self.COINDB_COMMIT_RE.match(line):
                    iso_str, txout_count = match.groups()
                    coindb_commit.append(
                        CoinDBCommitEntry(
                            timestamp=self.parse_timestamp(iso_str),
                            txout_count=int(txout_count),
                        )
                    )

        return ParsedLog(
            update_tip=update_tip,
            leveldb_compact=leveldb_compact,
            leveldb_gen_table=leveldb_gen_table,
            validation_txadd=validation_txadd,
            coindb_write_batch=coindb_write_batch,
            coindb_commit=coindb_commit,
        )


class PlotGenerator:
    """Generate performance plots from parsed log data."""

    def __init__(self, commit: str, output_dir: Path):
        self.commit = commit
        self.output_dir = output_dir
        self.generated_plots: list[Path] = []

        if not HAS_MATPLOTLIB:
            raise RuntimeError(
                "matplotlib is required for plot generation. "
                "Install with: pip install matplotlib"
            )

    def generate_all(self, data: ParsedLog) -> list[Path]:
        """Generate all plots from parsed data."""
        if not data.update_tip:
            logger.warning("No UpdateTip entries found, skipping plot generation")
            return []

        # Verify entries are sorted by time
        for i in range(len(data.update_tip) - 1):
            if data.update_tip[i].timestamp > data.update_tip[i + 1].timestamp:
                logger.warning("UpdateTip entries are not sorted by time")
                break

        # Extract base time for elapsed calculations
        base_time = data.update_tip[0].timestamp

        # Extract data series
        times = [e.timestamp for e in data.update_tip]
        heights = [e.height for e in data.update_tip]
        tx_counts = [e.tx_count for e in data.update_tip]
        cache_sizes = [e.cache_size_mb for e in data.update_tip]
        cache_counts = [e.cache_coins_count for e in data.update_tip]
        elapsed_minutes = [(t - base_time).total_seconds() / 60 for t in times]

        # Generate core plots
        self._plot(
            elapsed_minutes,
            heights,
            "Elapsed minutes",
            "Block Height",
            "Block Height vs Time",
            f"{self.commit}-height_vs_time.png",
        )

        self._plot(
            heights,
            cache_sizes,
            "Block Height",
            "Cache Size (MiB)",
            "Cache Size vs Block Height",
            f"{self.commit}-cache_vs_height.png",
            is_height_based=True,
        )

        self._plot(
            elapsed_minutes,
            cache_sizes,
            "Elapsed minutes",
            "Cache Size (MiB)",
            "Cache Size vs Time",
            f"{self.commit}-cache_vs_time.png",
        )

        self._plot(
            heights,
            tx_counts,
            "Block Height",
            "Transaction Count",
            "Transactions vs Block Height",
            f"{self.commit}-tx_vs_height.png",
            is_height_based=True,
        )

        self._plot(
            heights,
            cache_counts,
            "Block Height",
            "Coins Cache Size",
            "Coins Cache Size vs Height",
            f"{self.commit}-coins_cache_vs_height.png",
            is_height_based=True,
        )

        # LevelDB plots
        if data.leveldb_compact:
            compact_minutes = [
                (e.timestamp - base_time).total_seconds() / 60
                for e in data.leveldb_compact
            ]
            self._plot(
                compact_minutes,
                [1] * len(compact_minutes),
                "Elapsed minutes",
                "LevelDB Compaction",
                "LevelDB Compaction Events vs Time",
                f"{self.commit}-leveldb_compact_vs_time.png",
            )

        if data.leveldb_gen_table:
            gen_minutes = [
                (e.timestamp - base_time).total_seconds() / 60
                for e in data.leveldb_gen_table
            ]
            gen_keys = [e.keys_count for e in data.leveldb_gen_table]
            gen_bytes = [e.bytes_count for e in data.leveldb_gen_table]

            self._plot(
                gen_minutes,
                gen_keys,
                "Elapsed minutes",
                "Number of keys",
                "LevelDB Keys Generated vs Time",
                f"{self.commit}-leveldb_gen_keys_vs_time.png",
            )

            self._plot(
                gen_minutes,
                gen_bytes,
                "Elapsed minutes",
                "Number of bytes",
                "LevelDB Bytes Generated vs Time",
                f"{self.commit}-leveldb_gen_bytes_vs_time.png",
            )

        # Validation plots
        if data.validation_txadd:
            txadd_minutes = [
                (e.timestamp - base_time).total_seconds() / 60
                for e in data.validation_txadd
            ]
            self._plot(
                txadd_minutes,
                [1] * len(txadd_minutes),
                "Elapsed minutes",
                "Transaction Additions",
                "Transaction Additions to Mempool vs Time",
                f"{self.commit}-validation_txadd_vs_time.png",
            )

        # CoinDB plots
        if data.coindb_write_batch:
            batch_minutes = [
                (e.timestamp - base_time).total_seconds() / 60
                for e in data.coindb_write_batch
            ]
            batch_sizes = [e.size_mb for e in data.coindb_write_batch]
            self._plot(
                batch_minutes,
                batch_sizes,
                "Elapsed minutes",
                "Batch Size MiB",
                "Coin Database Partial/Final Write Batch Size vs Time",
                f"{self.commit}-coindb_write_batch_size_vs_time.png",
            )

        if data.coindb_commit:
            commit_minutes = [
                (e.timestamp - base_time).total_seconds() / 60
                for e in data.coindb_commit
            ]
            commit_txouts = [e.txout_count for e in data.coindb_commit]
            self._plot(
                commit_minutes,
                commit_txouts,
                "Elapsed minutes",
                "Transaction Output Count",
                "Coin Database Transaction Output Committed vs Time",
                f"{self.commit}-coindb_commit_txout_vs_time.png",
            )

        return self.generated_plots

    def _plot(
        self,
        x: list,
        y: list,
        x_label: str,
        y_label: str,
        title: str,
        filename: str,
        is_height_based: bool = False,
    ) -> None:
        """Generate a single plot."""
        if not x or not y:
            logger.debug(f"Skipping plot '{title}' - no data")
            return

        plt.figure(figsize=(30, 10))
        plt.plot(x, y)
        plt.title(title, fontsize=20)
        plt.xlabel(x_label, fontsize=16)
        plt.ylabel(y_label, fontsize=16)
        plt.grid(True)

        min_x, max_x = min(x), max(x)
        if min_x < max_x:
            plt.xlim(min_x, max_x)

        # Add fork markers for height-based plots
        if is_height_based:
            self._add_fork_markers(min_x, max_x, max(y))

        plt.xticks(rotation=90, fontsize=12)
        plt.yticks(fontsize=12)
        plt.tight_layout()

        output_path = self.output_dir / filename
        plt.savefig(output_path)
        plt.close()

        self.generated_plots.append(output_path)
        logger.info(f"Saved plot: {output_path}")

    def _add_fork_markers(self, min_x: float, max_x: float, max_y: float) -> None:
        """Add vertical lines for Bitcoin forks."""
        text_positions = {}
        position_increment = max_y * 0.05
        current_position = max_y * 0.9

        for fork_name, height in FORK_HEIGHTS.items():
            if min_x <= height <= max_x:
                plt.axvline(
                    x=height,
                    color=FORK_COLORS[fork_name],
                    linestyle=FORK_STYLES[fork_name],
                )

                if height in text_positions:
                    text_positions[height] -= position_increment
                else:
                    text_positions[height] = current_position
                    current_position -= position_increment
                    if current_position < max_y * 0.1:
                        current_position = max_y * 0.9

                plt.text(
                    height,
                    text_positions[height],
                    f"{fork_name} ({height})",
                    rotation=90,
                    verticalalignment="top",
                    color=FORK_COLORS[fork_name],
                )


class AnalyzePhase:
    """Analyze benchmark results and generate plots."""

    def run(
        self,
        commit: str,
        log_file: Path,
        output_dir: Path,
    ) -> AnalyzeResult:
        """Analyze a debug.log and generate plots.

        Args:
            commit: Commit hash (for naming)
            log_file: Path to debug.log
            output_dir: Where to save plots

        Returns:
            AnalyzeResult with paths to generated plots
        """
        if not HAS_MATPLOTLIB:
            raise RuntimeError(
                "matplotlib is required for plot generation. "
                "Install with: pip install matplotlib"
            )

        if not log_file.exists():
            raise FileNotFoundError(f"Log file not found: {log_file}")

        output_dir.mkdir(parents=True, exist_ok=True)

        logger.info(f"Parsing log file: {log_file}")
        parser = LogParser()
        data = parser.parse_file(log_file)

        # Log parsed data summary
        logger.info(f"  UpdateTip entries: {len(data.update_tip)}")
        logger.info(f"  LevelDB compact entries: {len(data.leveldb_compact)}")
        logger.info(f"  LevelDB gen table entries: {len(data.leveldb_gen_table)}")
        logger.info(f"  Validation txadd entries: {len(data.validation_txadd)}")
        logger.info(f"  CoinDB write batch entries: {len(data.coindb_write_batch)}")
        logger.info(f"  CoinDB commit entries: {len(data.coindb_commit)}")

        logger.info(f"Generating plots for {commit[:12]}")
        logger.info(f"  Output directory: {output_dir}")
        generator = PlotGenerator(commit[:12], output_dir)
        plots = generator.generate_all(data)

        logger.info(f"Generated {len(plots)} plots")

        return AnalyzeResult(
            commit=commit,
            output_dir=output_dir,
            plots=plots,
        )
