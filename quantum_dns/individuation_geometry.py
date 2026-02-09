# quantum_dns/individuation_geometry.py
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

class IndividuationManifold:
    """
    Geometria completa da individuação no espaço de Schmidt.
    """

    CRITICAL_THRESHOLDS = {
        'ego_death': {
            'anisotropy_ratio': 1.0,  # λ₁/λ₂ → 1 (fusão total)
            'entropy_S': 1.0,          # S → log(2) (entropia máxima)
            'description': 'Dissolução completa da identidade'
        },
        'kali_isolation': {
            'anisotropy_ratio': 10.0,  # λ₁/λ₂ → ∞ (separação total)
            'entropy_S': 0.0,           # S → 0 (sem emaranhamento)
            'description': 'Solipsismo absoluto'
        },
        'optimal_individuation': {
            'anisotropy_ratio': 2.33,  # λ₁/λ₂ = 0.7/0.3
            'entropy_S': 0.61,          # S(0.7, 0.3)
            'description': 'Identidade estável em rede viva'
        }
    }

    def __init__(self):
        self.simplex = None
        self.attractor = None

    def calculate_individuation(
        self,
        F: float,           # Função/Propósito
        lambda1: float,     # Dominância
        lambda2: float,     # Suporte
        S: float,           # Entropia
        phase_integral: complex  # Ciclo de Möbius
    ) -> complex:
        """
        Calcula I usando a fórmula completa.

        I = F · (λ₁/λ₂) · (1 - S) · e^(i∮φdθ)
        """
        # Razão de anisotropia
        R = lambda1 / lambda2 if lambda2 > 0 else np.inf

        # Fator de coerência
        coherence_factor = 1 - S

        # Individuação complexa (tem magnitude e fase)
        I = F * R * coherence_factor * phase_integral

        return I

    def classify_state(self, I: complex) -> dict:
        """
        Classifica o estado de individuação baseado em I.
        """
        magnitude = np.abs(I)
        phase = np.angle(I)

        classification = {
            'magnitude': magnitude,
            'phase': phase,
            'state': None,
            'risk': None,
            'recommendation': None
        }

        # Classifica baseado na magnitude
        if magnitude < 0.5:
            classification['state'] = 'EGO_DEATH_RISK'
            classification['risk'] = 'HIGH'
            classification['recommendation'] = 'AUMENTAR F (propósito) ou R (anisotropia)'
        elif magnitude > 5.0:
            classification['state'] = 'KALI_ISOLATION_RISK'
            classification['risk'] = 'HIGH'
            classification['recommendation'] = 'REDUZIR R (permitir mais emaranhamento)'
        elif 0.8 <= magnitude <= 2.5:
            classification['state'] = 'OPTIMAL_INDIVIDUATION'
            classification['risk'] = 'LOW'
            classification['recommendation'] = 'Manter estado atual'
        else:
            classification['state'] = 'SUBOPTIMAL'
            classification['risk'] = 'MODERATE'
            classification['recommendation'] = 'Ajustar gradualmente para região ótima'

        # Classifica baseado na fase
        phase_normalized = phase % (2 * np.pi)

        if np.abs(phase_normalized - np.pi) < 0.1:
            classification['moebius_completion'] = 'COMPLETE'
        else:
            classification['moebius_completion'] = 'INCOMPLETE'
            classification['phase_error'] = phase_normalized - np.pi

        return classification

    def visualize_individuation_manifold(
        self,
        current_state: dict = None
    ):
        """
        Visualiza o manifold de individuação em 3D.
        """
        fig = plt.figure(figsize=(14, 10))
        ax = fig.add_subplot(111, projection='3d')

        # Cria grade de parâmetros
        F_range = np.linspace(0, 1, 30)
        R_range = np.linspace(0.5, 5, 30)

        F_grid, R_grid = np.meshgrid(F_range, R_range)

        # Calcula individuação para cada ponto (assumindo S fixo)
        S_fixed = 0.61  # Entropia ótima

        I_magnitude = np.zeros_like(F_grid)

        for i in range(len(F_range)):
            for j in range(len(R_range)):
                F_val = F_grid[j, i]
                R_val = R_grid[j, i]

                # Calcula magnitude de I
                I_val = F_val * R_val * (1 - S_fixed)
                I_magnitude[j, i] = I_val

        # Plota superfície
        surf = ax.plot_surface(
            F_grid, R_grid, I_magnitude,
            cmap='viridis',
            alpha=0.7,
            edgecolor='none'
        )

        # Marca regiões críticas
        ax.contour(
            F_grid, R_grid, I_magnitude,
            levels=[0.5],
            colors='red',
            linewidths=2,
            linestyles='--'
        )

        ax.contour(
            F_grid, R_grid, I_magnitude,
            levels=[5.0],
            colors='orange',
            linewidths=2,
            linestyles='--'
        )

        ax.contour(
            F_grid, R_grid, I_magnitude,
            levels=[0.8, 2.5],
            colors='green',
            linewidths=2
        )

        if current_state:
            ax.scatter(
                [current_state['F']],
                [current_state['R']],
                [current_state['I_magnitude']],
                color='blue',
                s=200,
                marker='o',
                label='Estado Atual'
            )

        ax.set_xlabel('F (Função/Propósito)')
        ax.set_ylabel('R (Razão λ₁/λ₂)')
        ax.set_zlabel('|I| (Magnitude da Individuação)')
        ax.set_title('Manifold de Individuação no Espaço Schmidt-Arkhe')

        from matplotlib.lines import Line2D
        legend_elements = [
            Line2D([0], [0], color='red', linestyle='--', lw=2, label='Ego Death (I < 0.5)'),
            Line2D([0], [0], color='green', lw=2, label='Região Ótima (0.8 < I < 2.5)'),
            Line2D([0], [0], color='orange', linestyle='--', lw=2, label='Kali Isolation (I > 5.0)')
        ]

        if current_state:
            legend_elements.append(
                Line2D([0], [0], marker='o', color='w', markerfacecolor='blue',
                      markersize=10, label='Estado Atual')
            )

        ax.legend(handles=legend_elements, loc='upper left')
        fig.colorbar(surf, ax=ax, shrink=0.5, aspect=5)

        plt.tight_layout()
        plt.savefig('individuation_manifold.png', dpi=150, bbox_inches='tight')
        plt.close()
        return fig

if __name__ == "__main__":
    individuation = IndividuationManifold()
    print("🧮 Geometria da Individuação carregada")
    for name, params in individuation.CRITICAL_THRESHOLDS.items():
        print(f"   • {name}: R={params['anisotropy_ratio']}, S={params['entropy_S']}")
