require 'docker'
require 'httparty'

class CoinContainer
  def create(options = {})
    default_options = {
      image: "peercoin/base",
      shutdown_at_exit: true,
      delete_at_exit: false,
      remove_addr_after_shutdown: true,
      remove_wallet_after_shutdown: false,
    }

    options = default_options.merge(options)

    links = options[:links]
    case links
    when Hash
      links = links.map { |link_name, alias_name| [link_name, alias_name] }
    when Array
      links = links.map do |n|
        name = case n
               when String then n
               when CoinContainer then n.name
               else raise "Unknown link: #{n.inspect}"
               end
        [name, name.sub(/^\//, '')]
      end
    when nil
      links = []
    else
      raise "Invalid links"
    end
    name = options[:name]

    connects = links.map do |linked_name, alias_name|
      upname = alias_name.upcase
      "-addnode=$#{upname}_PORT_9903_TCP_ADDR:$#{upname}_PORT_9903_TCP_PORT"
    end

    default_args = {
      testnet: true,
      printtoconsole: true,
      rpcuser: 'bob',
      rpcpassword: 'bar',
      rpcallowip: '*.*.*.*',
      logtimestamps: true,
      keypool: 1,
      stakegen: false,
      dnsseed: false,
    }

    args = default_args.merge(options[:args] || {})

    cmd_args = args.map do |key, value|
      case value
      when true
        "-#{key}"
      when false
        "-#{key}=0"
      when Numeric
        "-#{key}=#{value}"
      else
        "-#{key}=\"#{value}\""
      end
    end
    cmd_args += connects

    bash_cmd = ""

    if options[:show_environment]
      bash_cmd += "echo Environment:; env; "
    end

    bash_cmd += "./ppcoind " + cmd_args.join(" ")

    if options[:remove_addr_after_shutdown]
      bash_cmd += "; rm /.ppcoin/testnet/addr.dat"
    end

    if options[:remove_wallet_after_shutdown]
      bash_cmd += "; rm /.ppcoin/testnet/wallet.dat"
    end

    command = [
      "stdbuf", "-oL", "-eL",
      '/bin/bash', '-c',
      bash_cmd,
    ]

    create_options = {
      'Image' => options[:image],
      'WorkingDir' => '/code',
      'Tty' => true,
      'Cmd' => command,
      'ExposedPorts' => {
        "9903/tcp" => {},
        "9904/tcp" => {},
      },
      'name' => name,
    }
    node_container = Docker::Container.create(create_options)

    if options[:shutdown_at_exit]
      at_exit do
        shutdown rescue nil
      end
    end
    if options[:delete_at_exit]
      at_exit do
        container.delete(force: true)
      end
    end

    node_container.start(
      'Binds' => ["#{File.expand_path('../../..', __FILE__)}:/code"],
      'PortBindings' => {
        "9904/tcp" => ['127.0.0.1'],
        "9903/tcp" => ['127.0.0.1'],
      },
      'Links' => links.map { |link_name, alias_name| "#{link_name}:#{alias_name}" },
    )

    @container = node_container
  end

  def load_data
    @json = @container.json
    @name = @json["Name"]

    ports = @json["NetworkSettings"]["Ports"]
    if ports.nil?
      raise "Unable to get port. Usualy this means the daemon process failed to start."
    end
    port = ports["9904/tcp"].first["HostPort"].to_i
    @rpc_port = port
    @port= ports["9903/tcp"].first["HostPort"].to_i
  end

  def initialize(options = {})
    if id = options[:id]
      get(id)
    else
      create(options)
    end
    load_data
  end

  def get(id)
    @container = Docker::Container.get(id)
  end

  attr_reader :rpc_port, :port, :container, :name

  def json
    container.json
  end

  def id
    @json["Id"]
  end

  def rpc(method, *params)
    data = {
      method: method,
      params: params,
      id: 'jsonrpc',
    }
    url = "http://localhost:#{rpc_port}/"
    auth = {
      username: "bob",
      password: "bar",
    }
    response = HTTParty.post url, body: data.to_json, headers: { 'Content-Type' => 'application/json' }, basic_auth: auth
    result = JSON.parse(response.body)
    raise result.inspect if result["error"]
    result["result"]
  end

  def wait_for_boot
    begin
      rpc("getinfo")
    rescue Errno::ECONNREFUSED, Errno::ECONNRESET, EOFError, Errno::EPIPE
      sleep 0.1
      retry
    end
  end

  def commit(repo)
    image = container.commit
    image.tag 'repo' => repo
  end

  def shutdown
    rpc("shutdown")
  end

  def wait_for_shutdown
    container.wait
  end

  def block_count
    rpc("getblockcount").to_i
  end

  def generate_stake(parent = nil)
    rpc("generatestake", *[parent].compact)
  end

  def top_hash
    rpc("getblockhash", rpc("getblockcount"))
  end

  def connection_count
    rpc("getconnectioncount").to_i
  end

  def info
    rpc "getinfo"
  end

  def new_address(account = "")
    rpc "getnewaddress", account
  end
end


