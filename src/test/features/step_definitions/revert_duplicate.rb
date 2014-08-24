When(/^node "(.*?)" sends a duplicate "(.*?)" of block "(.*?)"$/) do |node, duplicate, original|
  @blocks[duplicate] = @nodes[node].rpc("duplicateblock", @blocks[original])
end

When(/^node "(.*?)" finds a block "(.*?)" on top of block "(.*?)"$/) do |node, block, parent|
  @blocks[block] = @nodes[node].generate_stake(@blocks[parent])
end
