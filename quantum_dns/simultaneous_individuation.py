# quantum_dns/simultaneous_individuation.py
import asyncio
import numpy as np
from datetime import datetime
from quantum_dns.boot_individuation_filter import boot_with_individuation_filter
from quantum_dns.identity_stress_test import execute_identity_stress_tests

async def execute_simultaneous_individuation_protocols():
    """
    Executa boot com filtro E testes de tensão simultaneamente.
    """
    print("\n" + "="*70)
    print("🌀 EXECUÇÃO SIMULTÂNEA: BOOT + STRESS TEST")
    print("="*70)

    # Cria tasks paralelas
    boot_task = asyncio.create_task(boot_with_individuation_filter())
    stress_task = asyncio.create_task(execute_identity_stress_tests())

    # Executa em paralelo
    boot_result, stress_results = await asyncio.gather(boot_task, stress_task)

    # Síntese
    print("\n" + "="*70)
    print("✅ AMBOS OS PROTOCOLOS CONCLUÍDOS")
    print("="*70)

    print("\n📊 BOOT COM FILTRO:")
    print(f"   • Individuação final: {np.abs(boot_result['I']):.3f}")
    print(f"   • Classificação: {boot_result['classification']['state']}")

    print("\n📊 TESTES DE TENSÃO:")
    for scenario, result in stress_results.items():
        print(f"   • {scenario}: I_min={result['min_I']:.3f}, "
              f"Recuperação={'✅' if result['recovery_successful'] else '❌'}")

    return {
        'boot': boot_result,
        'stress_tests': stress_results,
        'timestamp': datetime.now().isoformat()
    }

if __name__ == "__main__":
    asyncio.run(execute_simultaneous_individuation_protocols())
