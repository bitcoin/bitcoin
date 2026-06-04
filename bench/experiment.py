"""Experiment planning and execution."""

from __future__ import annotations

import logging
import tomllib
from dataclasses import dataclass, field, replace
from pathlib import Path
from typing import Any

from .analyze import AnalyzePhase
from .benchmark import BenchmarkPhase, BenchmarkResult
from .benchmark_config import BenchmarkConfig
from .build import BuildPhase, BuiltBinary
from .config import Config, build_config
from .flamegraph import DifferentialFlamegraphPhase

logger = logging.getLogger(__name__)


def _as_instrumentation(value: Any) -> str:
    """Normalize a manifest instrumentation value."""
    if isinstance(value, bool):
        return "instrumented" if value else "uninstrumented"
    if value in ("instrumented", "uninstrumented"):
        return value
    raise ValueError(
        "instrumentation must be true, false, 'instrumented', or 'uninstrumented'"
    )


@dataclass(frozen=True)
class Subject:
    """A binary input to benchmark."""

    name: str
    commit: str | None = None
    binary: Path | None = None

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Subject:
        name = data.get("name")
        if not name:
            raise ValueError("subject missing required field: name")

        commit = data.get("commit")
        binary = Path(data["binary"]) if data.get("binary") else None
        if bool(commit) == bool(binary):
            raise ValueError(
                f"subject '{name}' must set exactly one of commit or binary"
            )

        return cls(name=name, commit=commit, binary=binary)


@dataclass(frozen=True)
class Profile:
    """A benchmark runtime profile."""

    name: str
    dbcache: int = 450
    instrumentation: str = "uninstrumented"
    runs: int | None = None
    bitcoind_args: dict[str, Any] = field(default_factory=dict)
    debug: list[str] | None = None
    outputs: list[str] = field(default_factory=list)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Profile:
        name = data.get("name")
        if not name:
            raise ValueError("profile missing required field: name")

        outputs = data.get("outputs", [])
        if isinstance(outputs, str):
            outputs = [outputs]

        debug = data.get("debug")
        if isinstance(debug, str):
            debug = [debug]

        return cls(
            name=name,
            dbcache=int(data.get("dbcache", 450)),
            instrumentation=_as_instrumentation(
                data.get("instrumentation", "uninstrumented")
            ),
            runs=data.get("runs"),
            bitcoind_args=dict(data.get("bitcoind", {})),
            debug=debug,
            outputs=list(outputs),
        )


@dataclass(frozen=True)
class Comparison:
    """A derived comparison between two subjects."""

    name: str
    before: str
    after: str
    profiles: list[str] = field(default_factory=list)
    outputs: list[str] = field(default_factory=list)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Comparison:
        name = data.get("name")
        before = data.get("before")
        after = data.get("after")
        if not name:
            raise ValueError("comparison missing required field: name")
        if not before or not after:
            raise ValueError(
                f"comparison '{name}' must set both before and after subjects"
            )

        profiles = data.get("profiles", [])
        if isinstance(profiles, str):
            profiles = [profiles]

        outputs = data.get("outputs", [])
        if isinstance(outputs, str):
            outputs = [outputs]

        return cls(
            name=name,
            before=before,
            after=after,
            profiles=list(profiles),
            outputs=list(outputs),
        )


@dataclass
class Experiment:
    """A full benchmark experiment manifest."""

    name: str
    full_ibd: bool = False
    start_height: int = 0
    runs: int = 3
    outputs: list[str] = field(default_factory=lambda: ["results"])
    bitcoind_args: dict[str, Any] = field(default_factory=dict)
    instrumented_debug: list[str] = field(default_factory=list)
    subjects: list[Subject] = field(default_factory=list)
    profiles: list[Profile] = field(default_factory=list)
    comparisons: list[Comparison] = field(default_factory=list)
    source_file: Path | None = None

    @classmethod
    def from_toml(cls, path: Path) -> Experiment:
        """Load an experiment manifest from TOML."""
        with open(path, "rb") as f:
            data = tomllib.load(f)

        experiment_data = data.get("experiment", {})
        benchmark_data = data.get("benchmark", {})
        bitcoind_data = data.get("bitcoind", {}).copy()

        instrumented = bitcoind_data.pop("instrumented", {})
        debug = instrumented.get("debug", [])

        outputs = experiment_data.get("outputs", ["results"])
        if isinstance(outputs, str):
            outputs = [outputs]

        subjects = [Subject.from_dict(item) for item in data.get("subjects", [])]
        subjects = [
            replace(subject, binary=path.parent / subject.binary)
            if subject.binary and not subject.binary.is_absolute()
            else subject
            for subject in subjects
        ]
        if not subjects:
            subjects = [Subject(name="head", commit="HEAD")]

        profiles = [Profile.from_dict(item) for item in data.get("profiles", [])]
        if not profiles:
            profiles = [Profile(name="450", dbcache=450)]

        comparisons = [
            Comparison.from_dict(item) for item in data.get("comparisons", [])
        ]

        return cls(
            name=experiment_data.get("name", path.stem),
            full_ibd=bool(benchmark_data.get("full_ibd", False)),
            start_height=0
            if benchmark_data.get("full_ibd", False)
            else int(benchmark_data.get("start_height", 0)),
            runs=int(benchmark_data.get("runs", 3)),
            outputs=list(outputs),
            bitcoind_args=bitcoind_data,
            instrumented_debug=list(debug),
            subjects=subjects,
            profiles=profiles,
            comparisons=comparisons,
            source_file=path,
        )

    def validate(self) -> list[str]:
        """Validate cross-references in the experiment."""
        errors = []
        subject_names = {subject.name for subject in self.subjects}
        profile_names = {profile.name for profile in self.profiles}

        if len(subject_names) != len(self.subjects):
            errors.append("subject names must be unique")
        if len(profile_names) != len(self.profiles):
            errors.append("profile names must be unique")

        for comparison in self.comparisons:
            if comparison.before not in subject_names:
                errors.append(
                    f"comparison '{comparison.name}' references unknown before "
                    f"subject '{comparison.before}'"
                )
            if comparison.after not in subject_names:
                errors.append(
                    f"comparison '{comparison.name}' references unknown after "
                    f"subject '{comparison.after}'"
                )
            for profile in comparison.profiles:
                if profile not in profile_names:
                    errors.append(
                        f"comparison '{comparison.name}' references unknown "
                        f"profile '{profile}'"
                    )

        return errors

    def profile_names_for(self, comparison: Comparison) -> list[str]:
        """Return the profile names selected by a comparison."""
        if comparison.profiles:
            return comparison.profiles
        return [profile.name for profile in self.profiles]


@dataclass
class RunArtifact:
    """Artifacts produced by one subject/profile run."""

    subject: Subject
    profile: Profile
    binary: Path
    output_dir: Path
    result: BenchmarkResult


@dataclass
class ExperimentResult:
    """Result of running an experiment."""

    output_dir: Path
    runs: dict[tuple[str, str], RunArtifact]
    comparisons: list[Path]


class ExperimentRunner:
    """Run an experiment manifest."""

    def __init__(
        self,
        config: Config,
        capabilities: Any,
        repo_path: Path | None = None,
    ):
        self.config = config
        self.capabilities = capabilities
        self.repo_path = repo_path or Path.cwd()

    def run(
        self,
        experiment: Experiment,
        datadir: Path | None,
        output_dir: Path,
        binaries_dir: Path,
        tmp_dir: Path | None = None,
        subject_names: list[str] | None = None,
        profile_names: list[str] | None = None,
    ) -> ExperimentResult:
        """Build subjects, run profiles, and derive experiment outputs."""
        errors = experiment.validate()
        if errors:
            raise ValueError("Invalid experiment:\n" + "\n".join(errors))

        subjects = self._selected_subjects(experiment, subject_names)
        profiles = self._selected_profiles(experiment, profile_names)
        comparisons = self._selected_comparisons(experiment, subjects)

        logger.info(f"Experiment: {experiment.name}")
        logger.info(f"  Subjects: {', '.join(s.name for s in subjects)}")
        logger.info(f"  Profiles: {', '.join(p.name for p in profiles)}")
        if comparisons:
            logger.info(
                f"  Comparisons: {', '.join(c.name for c in comparisons)}"
            )

        if self.config.dry_run:
            self._log_plan(subjects, profiles, comparisons, output_dir, binaries_dir)
            return ExperimentResult(output_dir=output_dir, runs={}, comparisons=[])

        output_dir.mkdir(parents=True, exist_ok=True)
        binaries_dir.mkdir(parents=True, exist_ok=True)

        binaries = self._resolve_binaries(subjects, binaries_dir)
        runs: dict[tuple[str, str], RunArtifact] = {}

        for subject in subjects:
            for profile in profiles:
                artifact = self._run_profile(
                    experiment=experiment,
                    subject=subject,
                    profile=profile,
                    binary=binaries[subject.name],
                    datadir=datadir,
                    output_dir=output_dir,
                    tmp_dir=tmp_dir,
                )
                runs[(subject.name, profile.name)] = artifact

        generated = self._derive_comparisons(
            experiment,
            comparisons,
            profiles,
            runs,
            output_dir,
        )

        return ExperimentResult(
            output_dir=output_dir,
            runs=runs,
            comparisons=generated,
        )

    def _resolve_binaries(
        self, subjects: list[Subject], binaries_dir: Path
    ) -> dict[str, BuiltBinary]:
        """Build commit subjects and collect binary subjects."""
        build_phase = BuildPhase(self.config, self.capabilities, self.repo_path)
        binaries: dict[str, BuiltBinary] = {}

        for subject in subjects:
            if subject.commit:
                build_result = build_phase.run(
                    f"{subject.commit}:{subject.name}",
                    output_dir=binaries_dir,
                )
                binaries[subject.name] = build_result.binary
            elif subject.binary:
                binaries[subject.name] = BuiltBinary(
                    name=subject.name,
                    path=subject.binary,
                    commit="",
                )
            else:
                raise AssertionError("validated subject has no binary source")

        return binaries

    def _run_profile(
        self,
        experiment: Experiment,
        subject: Subject,
        profile: Profile,
        binary: BuiltBinary,
        datadir: Path | None,
        output_dir: Path,
        tmp_dir: Path | None,
    ) -> RunArtifact:
        """Run one subject/profile pair."""
        run_name = f"{subject.name}-{profile.name}"
        run_output_dir = output_dir / "runs" / profile.name / subject.name
        run_tmp_dir = (
            tmp_dir / profile.name / subject.name
            if tmp_dir
            else run_output_dir / "tmp-datadir"
        )

        benchmark_config = self._benchmark_config_for(experiment, profile)
        benchmark_datadir = None if experiment.full_ibd else datadir
        config = self._config_for(
            profile=profile,
            runs=benchmark_config.runs,
            datadir=benchmark_datadir,
            tmp_datadir=run_tmp_dir,
            output_dir=run_output_dir,
        )

        errors = config.validate()
        if errors:
            raise ValueError("Invalid run config:\n" + "\n".join(errors))

        phase = BenchmarkPhase(config, self.capabilities, benchmark_config)
        result = phase.run(
            binary=(run_name, binary.path),
            datadir=benchmark_datadir,
            output_dir=run_output_dir,
        )

        if (
            profile.instrumentation == "instrumented"
            and "debug-plots" in self._outputs_for(experiment, profile)
            and result.debug_log
        ):
            AnalyzePhase().run(
                commit=run_name,
                log_file=result.debug_log,
                output_dir=run_output_dir / "plots",
            )

        return RunArtifact(
            subject=subject,
            profile=profile,
            binary=binary.path,
            output_dir=run_output_dir,
            result=result,
        )

    def _derive_comparisons(
        self,
        experiment: Experiment,
        comparisons: list[Comparison],
        profiles: list[Profile],
        runs: dict[tuple[str, str], RunArtifact],
        output_dir: Path,
    ) -> list[Path]:
        """Derive comparison artifacts."""
        generated: list[Path] = []
        selected_profiles = {profile.name for profile in profiles}

        for comparison in comparisons:
            outputs = comparison.outputs or experiment.outputs
            if "differential-flamegraph" not in outputs:
                continue

            for profile_name in experiment.profile_names_for(comparison):
                if profile_name not in selected_profiles:
                    continue
                before = runs[(comparison.before, profile_name)]
                after = runs[(comparison.after, profile_name)]
                if not before.result.perf_data or not after.result.perf_data:
                    raise RuntimeError(
                        f"comparison '{comparison.name}' profile '{profile_name}' "
                        "requires instrumented runs with perf.data artifacts"
                    )

                diff_dir = output_dir / "comparisons" / comparison.name / profile_name
                result = DifferentialFlamegraphPhase(self.capabilities).run(
                    before_perf=before.result.perf_data,
                    after_perf=after.result.perf_data,
                    output_dir=diff_dir,
                    before_name=before.result.name,
                    after_name=after.result.name,
                )
                generated.extend([result.before_svg, result.after_svg])

        return generated

    def _selected_subjects(
        self, experiment: Experiment, names: list[str] | None
    ) -> list[Subject]:
        """Return subjects selected by optional scheduler filters."""
        if not names:
            return experiment.subjects

        selected = [subject for subject in experiment.subjects if subject.name in names]
        missing = sorted(set(names) - {subject.name for subject in selected})
        if missing:
            raise ValueError(f"unknown subject filter(s): {', '.join(missing)}")
        return selected

    def _selected_profiles(
        self, experiment: Experiment, names: list[str] | None
    ) -> list[Profile]:
        """Return profiles selected by optional scheduler filters."""
        if not names:
            return experiment.profiles

        selected = [profile for profile in experiment.profiles if profile.name in names]
        missing = sorted(set(names) - {profile.name for profile in selected})
        if missing:
            raise ValueError(f"unknown profile filter(s): {', '.join(missing)}")
        return selected

    def _selected_comparisons(
        self,
        experiment: Experiment,
        subjects: list[Subject],
    ) -> list[Comparison]:
        """Return comparisons that can be derived from selected runs."""
        subject_names = {subject.name for subject in subjects}
        selected = []

        for comparison in experiment.comparisons:
            if comparison.before not in subject_names or comparison.after not in subject_names:
                continue
            selected.append(comparison)

        return selected

    def _benchmark_config_for(
        self, experiment: Experiment, profile: Profile
    ) -> BenchmarkConfig:
        """Build a BenchmarkConfig for one profile."""
        bitcoind_args = {**experiment.bitcoind_args, **profile.bitcoind_args}
        debug = (
            profile.debug if profile.debug is not None else experiment.instrumented_debug
        )

        return BenchmarkConfig(
            full_ibd=experiment.full_ibd,
            start_height=experiment.start_height,
            runs=profile.runs or experiment.runs,
            matrix={},
            bitcoind_args=bitcoind_args,
            instrumented_debug=debug,
            source_file=experiment.source_file,
        )

    def _config_for(
        self,
        profile: Profile,
        runs: int,
        datadir: Path | None,
        tmp_datadir: Path,
        output_dir: Path,
    ) -> Config:
        """Build low-level Config for one run."""
        return build_config(
            cli_args={
                "datadir": str(datadir) if datadir else None,
                "tmp_datadir": str(tmp_datadir),
                "output_dir": str(output_dir),
                "no_cache_drop": self.config.no_cache_drop,
                "dry_run": self.config.dry_run,
                "verbose": self.config.verbose,
                "runs": runs,
                "dbcache": profile.dbcache,
                "instrumented": profile.instrumentation,
            },
            profile=self.config.profile,
        )

    def _outputs_for(self, experiment: Experiment, profile: Profile) -> list[str]:
        """Return profile outputs with experiment defaults applied."""
        return profile.outputs or experiment.outputs

    def _log_plan(
        self,
        subjects: list[Subject],
        profiles: list[Profile],
        comparisons: list[Comparison],
        output_dir: Path,
        binaries_dir: Path,
    ) -> None:
        """Log the tasks that would run."""
        logger.info("[DRY RUN] Would write outputs to: %s", output_dir)
        logger.info("[DRY RUN] Would write binaries to: %s", binaries_dir)
        for subject in subjects:
            if subject.commit:
                logger.info(
                    "[DRY RUN] Would build subject %s from %s",
                    subject.name,
                    subject.commit,
                )
            else:
                logger.info(
                    "[DRY RUN] Would use subject %s binary at %s",
                    subject.name,
                    subject.binary,
                )
        for subject in subjects:
            for profile in profiles:
                logger.info(
                    "[DRY RUN] Would run %s with profile %s",
                    subject.name,
                    profile.name,
                )
        for comparison in comparisons:
            logger.info("[DRY RUN] Would derive comparison %s", comparison.name)
