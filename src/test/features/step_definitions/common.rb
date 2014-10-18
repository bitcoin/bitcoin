Before do
  @blocks = {}
  @addresses = {}
  @nodes = {}
  @tx = {}
end

Given(/^a network with nodes? (.+) able to mint$/) do |node_names|
  node_names = node_names.scan(/"(.*?)"/).map(&:first)
  available_nodes = %w( a b c d e )
  raise "More than #{available_nodes.size} nodes not supported" if node_names.size > available_nodes.size
  @nodes = {}

  node_names.each_with_index do |name, i|
    options = {
      image: "peercoinnet/#{available_nodes[i]}",
      links: @nodes.values.map(&:name),
      args: {
        debug: true,
        timetravel: 30*24*3600,
      },
    }
    node = CoinContainer.new(options)
    @nodes[name] = node
    node.wait_for_boot
  end

  wait_for(10) do
    @nodes.values.all? do |node|
      count = node.connection_count
      count == @nodes.size - 1
    end
  end
  wait_for do
    @nodes.values.map do |node|
      count = node.block_count
      count
    end.uniq.size == 1
  end
end

After do
  if @nodes
    require 'thread'
    @nodes.values.reverse.map do |node|
      Thread.new do
        node.shutdown
        #node.wait_for_shutdown
        #begin
        #  node.container.delete(force: true)
        #rescue
        #end
      end
    end.each(&:join)
  end
end

When(/^node "(.*?)" finds a block "([^"]*?)"$/) do |node, block|
  @blocks[block] = @nodes[node].generate_stake
end

When(/^node "(.*?)" finds a block$/) do |node|
  @nodes[node].generate_stake
end

Then(/^all nodes should be at block "(.*?)"$/) do |block|
  begin
    wait_for do
      main = @nodes.values.map(&:top_hash)
      main.all? { |hash| hash == @blocks[block] }
    end
  rescue
    raise "Not at block #{block}: #{@nodes.values.map(&:top_hash).map { |hash| @blocks.key(hash) }.inspect}"
  end
end

Given(/^all nodes reach the same height$/) do
  wait_for do
    expect(@nodes.values.map(&:block_count).uniq.size).to eq(1)
  end
end

When(/^node "(.*?)" sends "(.*?)" to "([^"]*?)" in transaction "(.*?)"$/) do |arg1, arg2, arg3, arg4|
  @tx[arg4] = @nodes[arg1].rpc "sendtoaddress", @addresses[arg3], parse_number(arg2)
end

When(/^node "(.*?)" sends "(.*?)" to "([^"]*?)"$/) do |arg1, arg2, arg3|
  @nodes[arg1].rpc "sendtoaddress", @addresses[arg3], parse_number(arg2)
end

When(/^node "(.*?)" finds a block received by all other nodes$/) do |arg1|
  node = @nodes[arg1]
  block = node.generate_stake
  wait_for do
    main = @nodes.values.map(&:top_hash)
    main.all? { |hash| hash == block }
  end
end

Given(/^node "(.*?)" generates a new address "(.*?)"$/) do |arg1, arg2|
  @addresses[arg2] = @nodes[arg1].rpc("getnewaddress")
end

When(/^node "(.*?)" sends "(.*?)" to "(.*?)" through transaction "(.*?)"$/) do |arg1, arg2, arg3, arg4|
  @tx[arg4] = @nodes[arg1].rpc "sendtoaddress", @addresses[arg3], arg2.to_f
end

Then(/^transaction "(.*?)" on node "(.*?)" should have (\d+) confirmations?$/) do |arg1, arg2, arg3|
  wait_for do
    expect(@nodes[arg2].rpc("gettransaction", @tx[arg1])["confirmations"]).to eq(arg3.to_i)
  end
end

Then(/^all nodes should (?:have|reach) (\d+) transactions? in memory pool$/) do |arg1|
  wait_for do
    expect(@nodes.values.map { |node| node.rpc("getmininginfo")["pooledtx"] }).to eq(@nodes.map { arg1.to_i })
  end
end
