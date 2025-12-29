import time
import threading
from test_framework.util import wait_until_helper_internal

# 1️⃣ Basic predicate success
def test_wait_until_basic():
    """Predicate becomes True after short delay."""
    x = {"value": False}

    def predicate():
        return x["value"]

    def flip_value_later():
        time.sleep(0.1)
        x["value"] = True

    t = threading.Thread(target=flip_value_later)
    t.start()

    wait_until_helper_internal(predicate, timeout=1)
    t.join()

    assert x["value"] is True


# 2️⃣ Predicate with retries
def test_wait_until_with_retries():
    """Predicate fails a few times, then succeeds."""
    counter = {"count": 0}

    def predicate():
        counter["count"] += 1
        return counter["count"] >= 3

    wait_until_helper_internal(predicate, timeout=1, check_interval=0.05, retries=2)

    assert counter["count"] >= 3


# 3️⃣ Adaptive timeout simulation
def test_wait_until_adaptive_timeout(monkeypatch):
    """Adaptive timeout should adjust based on simulated load."""
    x = {"value": False}

    def predicate():
        return x["value"]

    # Simulate loadavg
    monkeypatch.setattr("os.getloadavg", lambda: (5.0, 0.0, 0.0))

    def flip_value_later():
        time.sleep(0.1)
        x["value"] = True

    t = threading.Thread(target=flip_value_later)
    t.start()

    wait_until_helper_internal(predicate, timeout=0.5, adaptive=True)
    t.join()

    assert x["value"] is True


# 4️⃣ Parallel predicates
def test_wait_until_parallel():
    """Multiple predicates evaluated in parallel, all must be True."""
    states = [{"value": False}, {"value": False}]

    def pred1():
        time.sleep(0.05)
        states[0]["value"] = True
        return states[0]["value"]

    def pred2():
        time.sleep(0.1)
        states[1]["value"] = True
        return states[1]["value"]

    wait_until_helper_internal([pred1, pred2], timeout=1)

    assert all(s["value"] for s in states)


# 5️⃣ Predicate history logging
def test_wait_until_history():
    """History should record timestamps and predicate results."""
    history = []
    x = {"value": False}

    def predicate():
        if not x["value"]:
            x["value"] = True
            return False
        return True

    wait_until_helper_internal(predicate, timeout=0.5, history=history, retries=1)

    assert len(history) > 0
    assert history[-1][1][0] is True  # Last result should be True
