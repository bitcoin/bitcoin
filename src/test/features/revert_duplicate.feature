Feature: The top block is removed if a duplicate stake is received

  Scenario: A node reuses the stake of the top block in another block
    Given a network with nodes "A", "B" and "C" able to mint
    When node "A" finds a block "X"
    Then all nodes should be at block "X"
    When node "B" finds a block "Y"
    Then all nodes should be at block "Y"
    When node "B" sends a duplicate "Y2" of block "Y"
    Then all nodes should be at block "X"
    When node "C" finds a block "T"
    Then all nodes should be at block "T"


  Scenario: A node builds a block on top of a block that was removed because of duplication
    Given a network with nodes "A", "B" and "C" able to mint
    And a node "D" connected only to node "A"

    When node "A" finds a block "X"
    Then all nodes should be at block "X"

    When node "B" finds a block "Y" not received by node "C"
    Then nodes "A", "B" and "D" should be at block "Y"
    And node "C" should be at block "X"

    When node "B" sends a duplicate "Y2" of block "Y"
    Then nodes "A", "B" and "D" should be at block "X"
    And node "C" should be at block "Y2"

    When node "C" finds a block "Z"
    Then all nodes should be at block "Z"
    And node "D" should have 1 connection


  Scenario: A node builds a block on top of a duplicated block
    Given a network with nodes "A", "B" and "C" able to mint
    And a node "D" connected only to node "A"

    When node "A" finds a block "X"
    Then all nodes should be at block "X"

    When node "B" finds a block "Y"
    Then all nodes should be at block "Y"

    When node "B" sends a duplicate "Y2" of block "Y" not received by node "C"
    Then nodes "A", "B" and "D" should be at block "X"
    And node "C" should be at block "Y"

    When node "C" finds a block "Z"
    Then all nodes should be at block "Z"
    And node "D" should have 1 connection


  Scenario: Transactions unconfirmed by duplicate removal gets confirmed again in the next block
    Given a network with nodes "A", "B" and "C" able to mint
    When node "A" finds a block "X"
    Then all nodes should be at block "X"

    When node "B" generates a new address "addr"
    And node "C" sends "1000" to "addr" through transaction "tx"
    Then all nodes should have 1 transaction in memory pool

    When node "A" finds a block "Y"
    Then all nodes should be at block "Y"
    And transaction "tx" on node "C" should have 1 confirmation
    And all nodes should have 0 transactions in memory pool

    When node "A" sends a duplicate "Y2" of block "Y"
    Then all nodes should be at block "X"
    And transaction "tx" on node "C" should have 0 confirmations
    And all nodes should have 1 transaction in memory pool

    When node "A" finds a block "Z"
    Then all nodes should be at block "Z"
    And transaction "tx" on node "C" should have 1 confirmations
    And all nodes should have 0 transactions in memory pool


  Scenario: A node sends a block with a duplicate stake of an already confirmed block
    Given a network with nodes "A", "B" and "C" able to mint
    When node "A" finds a block "X"
    Then all nodes should be at block "X"
    When node "B" finds a block "Y"
    Then all nodes should be at block "Y"
    When node "A" sends a duplicate "X2" of block "X"
    Then all nodes should be at block "Y"


  Scenario: A node sends a block with a duplicate stake but without a valid signature because it doesn't have the associated private key
    Given a network with nodes "A", "B" and "C" able to mint
    When node "A" finds a block "X"
    Then all nodes should be at block "X"
    When node "B" finds a block "Y"
    Then all nodes should be at block "Y"
    When node "C" sends a duplicate "Y2" of block "Y"
    Then all nodes should be at block "Y"
