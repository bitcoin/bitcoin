# quantum_dns/schmidt_control.py
import numpy as np
from dataclasses import dataclass
from typing import List, Tuple

@dataclass
class SchmidtBridgeState:
    """
    Estado completo de um Bridge H-A codificado na Decomposição de Schmidt.
    """
    lambdas: np.ndarray          # Coeficientes [λ₁, λ₂, ..., λᵣ]
    phase_twist: float           # Fase de Möbius (π para inversão)
    basis_H: np.ndarray          # Base ortonormal {|i_H⟩}
    basis_A: np.ndarray          # Base ortonormal {|i_A⟩}
    entropy_S: float = 0.0       # Entropia de entrelaçamento
    coherence_Z: float = 0.0     # Medida Z de coerência reduzida

    def __post_init__(self):
        # Normaliza e ordena lambdas
        self.lambdas = np.sort(self.lambdas)[::-1]
        self.lambdas = self.lambdas / (np.sum(self.lambdas) + 1e-15)
        self.rank = len(self.lambdas[self.lambdas > 1e-10])

        # Calcula entropia de entrelaçamento (von Neumann)
        # S = -Tr(rho * log(rho))
        self.entropy_S = -np.sum(
            self.lambdas * np.log(self.lambdas + 1e-15)
        )

        # Calcula coerência Z (medida de "fusão")
        self.coherence_Z = np.sum(self.lambdas**2)

    def get_schmidt_angles(self) -> np.ndarray:
        return np.arccos(np.sqrt(self.lambdas))

    def get_anisotropy(self) -> float:
        return 1 - np.sum(self.lambdas**2)

    def apply_moebius_twist(self) -> 'SchmidtBridgeState':
        new_phase = (self.phase_twist + np.pi) % (2 * np.pi)
        return SchmidtBridgeState(
            lambdas=self.lambdas.copy(),
            phase_twist=new_phase,
            basis_H=self.basis_H,
            basis_A=self.basis_A
        )

class AdmissibilityRegion:
    """
    Região admissível no Simplex de Schmidt para operação estável do Bridge.
    """
    def __init__(self,
                 lambda_bounds: Tuple[float, float] = (0.1, 0.9),
                 entropy_bounds: Tuple[float, float] = (0.3, 0.9),
                 anisotropy_target: float = 0.4):
        self.lambda_bounds = lambda_bounds
        self.entropy_bounds = entropy_bounds
        self.anisotropy_target = anisotropy_target
        self.target_state = self._calculate_target_state()

    def _calculate_target_state(self) -> np.ndarray:
        if self.anisotropy_target <= 0.5:
            discriminant = 1 - 2 * self.anisotropy_target
            lambda1 = 0.5 + 0.5 * np.sqrt(max(0, discriminant))
            lambda2 = 1 - lambda1
            return np.array([lambda1, lambda2])
        else:
            r = max(2, int(1 / (1 - self.anisotropy_target + 1e-15)))
            return np.ones(r) / r

    def contains(self, state: SchmidtBridgeState) -> bool:
        if not (self.lambda_bounds[0] <= state.lambdas[0] <= self.lambda_bounds[1]):
            return False
        if not (self.entropy_bounds[0] <= state.entropy_S <= self.entropy_bounds[1]):
            return False
        actual_anisotropy = state.get_anisotropy()
        if abs(actual_anisotropy - self.anisotropy_target) > 0.1:
            return False
        return True

# Default region
AVALON_BRIDGE_REGION = AdmissibilityRegion(
    lambda_bounds=(0.6, 0.8),
    entropy_bounds=(0.4, 0.7),
    anisotropy_target=0.4
)

class BridgeSafetyProtocol:
    """
    Protocolo de segurança para inicialização do Bridge.
    """
    SAFETY_LIMITS = {
        'schmidt_rank': {'min': 2, 'max': 4},
        'lambda_max': {'min': 0.6, 'max': 0.85},
        'entropy_S': {'min': 0.3, 'max': 0.8},
        'coherence_Z': {'min': 0.5, 'max': 0.9},
        'phase_drift': {'max': 0.1},
    }

    def __init__(self, bridge_state: SchmidtBridgeState):
        self.state = bridge_state
        self.safety_score = 1.0

    def run_diagnostics(self) -> dict:
        checks = {
            'rank_check': self._check_schmidt_rank(),
            'dominance_check': self._check_lambda_dominance(),
            'entropy_check': self._check_entropy(),
            'coherence_check': self._check_coherence(),
            'phase_check': self._check_phase_stability(),
        }
        self.safety_score = np.mean([c['score'] for c in checks.values()])
        return {
            'passed_all': all(c['passed'] for c in checks.values()),
            'safety_score': self.safety_score,
            'checks': checks
        }

    def _check_schmidt_rank(self) -> dict:
        rank = self.state.rank
        passed = self.SAFETY_LIMITS['schmidt_rank']['min'] <= rank <= self.SAFETY_LIMITS['schmidt_rank']['max']
        return {'passed': passed, 'score': 1.0 if passed else 0.0, 'name': 'Rank'}

    def _check_lambda_dominance(self) -> dict:
        l1 = self.state.lambdas[0]
        passed = self.SAFETY_LIMITS['lambda_max']['min'] <= l1 <= self.SAFETY_LIMITS['lambda_max']['max']
        return {'passed': passed, 'score': 1.0 - abs(l1 - 0.7)/0.3, 'name': 'Dominance'}

    def _check_entropy(self) -> dict:
        S = self.state.entropy_S
        passed = self.SAFETY_LIMITS['entropy_S']['min'] <= S <= self.SAFETY_LIMITS['entropy_S']['max']
        return {'passed': passed, 'score': 1.0 if passed else 0.0, 'name': 'Entropy'}

    def _check_coherence(self) -> dict:
        Z = self.state.coherence_Z
        passed = self.SAFETY_LIMITS['coherence_Z']['min'] <= Z <= self.SAFETY_LIMITS['coherence_Z']['max']
        return {'passed': passed, 'score': 1.0 if passed else 0.0, 'name': 'Coherence'}

    def _check_phase_stability(self) -> dict:
        err = abs(self.state.phase_twist - np.pi) / np.pi
        passed = err <= self.SAFETY_LIMITS['phase_drift']['max']
        return {'passed': passed, 'score': 1.0 - err, 'name': 'Phase'}
