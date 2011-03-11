
    

  

<!DOCTYPE html>
<html>
  <head>
    <meta charset='utf-8'>
    <meta http-equiv="X-UA-Compatible" content="chrome=1">
        <title>net.cpp at f0fcb982f7f6a5184df47250b2396d92476c7d99 from TheBlueMatt/bitcoin - GitHub</title>
    <link rel="search" type="application/opensearchdescription+xml" href="/opensearch.xml" title="GitHub" />
    <link rel="fluid-icon" href="https://github.com/fluidicon.png" title="GitHub" />

    <link href="https://assets1.github.com/stylesheets/bundle_common.css?2b893794092cd44a789db64b3da95d1091187f55" media="screen" rel="stylesheet" type="text/css" />
<link href="https://assets0.github.com/stylesheets/bundle_github.css?2b893794092cd44a789db64b3da95d1091187f55" media="screen" rel="stylesheet" type="text/css" />

    <script type="text/javascript">
      if (typeof console == "undefined" || typeof console.log == "undefined")
        console = { log: function() {} }
    </script>
    <script type="text/javascript" charset="utf-8">
      var GitHub = {}
      var github_user = 'TheBlueMatt'
      
    </script>
    <script src="https://assets3.github.com/javascripts/jquery/jquery-1.4.2.min.js?2b893794092cd44a789db64b3da95d1091187f55" type="text/javascript"></script>
    <script src="https://assets3.github.com/javascripts/bundle_common.js?2b893794092cd44a789db64b3da95d1091187f55" type="text/javascript"></script>
<script src="https://assets0.github.com/javascripts/bundle_github.js?2b893794092cd44a789db64b3da95d1091187f55" type="text/javascript"></script>


        <script type="text/javascript" charset="utf-8">
      GitHub.spy({
        repo: "TheBlueMatt/bitcoin"
      })
    </script>

    
  <link href="https://github.com/TheBlueMatt/bitcoin/commits/f0fcb982f7f6a5184df47250b2396d92476c7d99.atom" rel="alternate" title="Recent Commits to bitcoin:f0fcb982f7f6a5184df47250b2396d92476c7d99" type="application/atom+xml" />

    <META NAME="ROBOTS" CONTENT="NOINDEX, FOLLOW">    <meta name="description" content="Bitcoin integration/staging tree" />
    <script type="text/javascript">
      GitHub.nameWithOwner = GitHub.nameWithOwner || "TheBlueMatt/bitcoin";
      GitHub.currentRef = '';
      GitHub.commitSHA = "f0fcb982f7f6a5184df47250b2396d92476c7d99";
      GitHub.currentPath = 'net.cpp';
      GitHub.masterBranch = "master";

      
    </script>
  

        <script type="text/javascript">
      var _gaq = _gaq || [];
      _gaq.push(['_setAccount', 'UA-3769691-2']);
      _gaq.push(['_setDomainName', 'none']);
      _gaq.push(['_trackPageview']);
      (function() {
        var ga = document.createElement('script');
        ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
        ga.setAttribute('async', 'true');
        document.documentElement.firstChild.appendChild(ga);
      })();
    </script>

    
  </head>

  

  <body class="logged_in page-blob">
    

    

    

    

    

    <div class="subnavd" id="main">
      <div id="header" class="true">
        
          <a class="logo boring" href="https://github.com/">
            <img alt="github" class="default" src="https://assets0.github.com/images/modules/header/logov3.png?2b893794092cd44a789db64b3da95d1091187f55" />
            <!--[if (gt IE 8)|!(IE)]><!-->
            <img alt="github" class="hover" src="https://assets2.github.com/images/modules/header/logov3-hover.png?2b893794092cd44a789db64b3da95d1091187f55" />
            <!--<![endif]-->
          </a>
        
        
          





  
    <div class="userbox">
      <div class="avatarname">
        <a href="https://github.com/TheBlueMatt"><img src="https://secure.gravatar.com/avatar/c0d69eae553e6965038ba73dde6dead7?s=140&d=https://assets.github.com%2Fimages%2Fgravatars%2Fgravatar-140.png" alt="" width="20" height="20"  /></a>
        <a href="https://github.com/TheBlueMatt" class="name">TheBlueMatt</a>

        
        
      </div>
      <ul class="usernav">
        <li><a href="https://github.com/">Dashboard</a></li>
        <li>
          
          <a href="https://github.com/inbox">Inbox</a>
          <a href="https://github.com/inbox" class="unread_count ">0</a>
        </li>
        <li><a href="https://github.com/account">Account Settings</a></li>
                <li><a href="/logout">Log Out</a></li>
      </ul>
    </div><!-- /.userbox -->
  


        
        <div class="topsearch">
  
    <form action="/search" id="top_search_form" method="get">
      <a href="/search" class="advanced-search tooltipped downwards" title="Advanced Search">Advanced Search</a>
      <input type="search" class="search my_repos_autocompleter" name="q" results="5" placeholder="Search&hellip;" /> <input type="submit" value="Search" class="button" />
      <input type="hidden" name="type" value="Everything" />
      <input type="hidden" name="repo" value="" />
      <input type="hidden" name="langOverride" value="" />
      <input type="hidden" name="start_value" value="1" />
    </form>
    <ul class="nav">
      <li><a href="/explore">Explore GitHub</a></li>
      <li><a href="https://gist.github.com">Gist</a></li>
      <li><a href="/blog">Blog</a></li>
      <li><a href="http://help.github.com">Help</a></li>
    </ul>
  
</div>

      </div>

      
      
        
    <div class="site">
      <div class="pagehead repohead vis-public fork  ">

      

      <div class="title-actions-bar">
        <h1>
          <a href="/TheBlueMatt">TheBlueMatt</a> / <strong><a href="https://github.com/TheBlueMatt/bitcoin">bitcoin</a></strong>
          
            <span class="fork-flag">
              
              <span class="text">forked from <a href="/bitcoin/bitcoin">bitcoin/bitcoin</a></span>
            </span>
          
          
        </h1>

        
    <ul class="actions">
      

      
        <li class="for-owner" style="display:none"><a href="https://github.com/TheBlueMatt/bitcoin/admin" class="minibutton btn-admin "><span><span class="icon"></span>Admin</span></a></li>
        <li>
          <a href="/TheBlueMatt/bitcoin/toggle_watch" class="minibutton btn-watch " id="watch_button" onclick="var f = document.createElement('form'); f.style.display = 'none'; this.parentNode.appendChild(f); f.method = 'POST'; f.action = this.href;var s = document.createElement('input'); s.setAttribute('type', 'hidden'); s.setAttribute('name', 'authenticity_token'); s.setAttribute('value', 'cf1c0b8959733617e26844d243ba9e11c32dfb89'); f.appendChild(s);f.submit();return false;" style="display:none"><span><span class="icon"></span>Watch</span></a>
          <a href="/TheBlueMatt/bitcoin/toggle_watch" class="minibutton btn-watch " id="unwatch_button" onclick="var f = document.createElement('form'); f.style.display = 'none'; this.parentNode.appendChild(f); f.method = 'POST'; f.action = this.href;var s = document.createElement('input'); s.setAttribute('type', 'hidden'); s.setAttribute('name', 'authenticity_token'); s.setAttribute('value', 'cf1c0b8959733617e26844d243ba9e11c32dfb89'); f.appendChild(s);f.submit();return false;" style="display:none"><span><span class="icon"></span>Unwatch</span></a>
        </li>
        
          
            <li class="for-notforked" style="display:none"><a href="/TheBlueMatt/bitcoin/fork" class="minibutton btn-fork " id="fork_button" onclick="var f = document.createElement('form'); f.style.display = 'none'; this.parentNode.appendChild(f); f.method = 'POST'; f.action = this.href;var s = document.createElement('input'); s.setAttribute('type', 'hidden'); s.setAttribute('name', 'authenticity_token'); s.setAttribute('value', 'cf1c0b8959733617e26844d243ba9e11c32dfb89'); f.appendChild(s);f.submit();return false;"><span><span class="icon"></span>Fork</span></a></li>
            <li class="for-hasfork" style="display:none"><a href="#" class="minibutton btn-fork " id="your_fork_button"><span><span class="icon"></span>Your Fork</span></a></li>
          

          <li id='pull_request_item' class='nspr' style='display:none'><a href="/TheBlueMatt/bitcoin/pull/new/f0fcb982f7f6a5184df47250b2396d92476c7d99" class="minibutton btn-pull-request "><span><span class="icon"></span>Pull Request</span></a></li>
        
      
      
      <li class="repostats">
        <ul class="repo-stats">
          <li class="watchers"><a href="/TheBlueMatt/bitcoin/watchers" title="Watchers" class="tooltipped downwards">1</a></li>
          <li class="forks"><a href="/TheBlueMatt/bitcoin/network" title="Forks" class="tooltipped downwards">48</a></li>
        </ul>
      </li>
    </ul>

      </div>

        
  <ul class="tabs">
    <li><a href="https://github.com/TheBlueMatt/bitcoin/tree/" class="selected" highlight="repo_source">Source</a></li>
    <li><a href="https://github.com/TheBlueMatt/bitcoin/commits/" highlight="repo_commits">Commits</a></li>
    <li><a href="/TheBlueMatt/bitcoin/network" highlight="repo_network">Network</a></li>
    <li><a href="/TheBlueMatt/bitcoin/pulls" highlight="repo_pulls">Pull Requests (0)</a></li>

    
      <li><a href="/TheBlueMatt/bitcoin/forkqueue" highlight="repo_fork_queue">Fork Queue</a></li>
    

    

                <li><a href="/TheBlueMatt/bitcoin/wiki" highlight="repo_wiki">Wiki (0)</a></li>
        
    <li><a href="/TheBlueMatt/bitcoin/graphs" highlight="repo_graphs">Graphs</a></li>

    <li class="contextswitch nochoices">
      <span class="toggle leftwards tooltipped" title="f0fcb982f7f6a5184df47250b2396d92476c7d99">
        <em>Tree:</em>
        <code>f0fcb98</code>
      </span>
    </li>
  </ul>

  <div style="display:none" id="pl-description"><p><em class="placeholder">click here to add a description</em></p></div>
  <div style="display:none" id="pl-homepage"><p><em class="placeholder">click here to add a homepage</em></p></div>

  <div class="subnav-bar">
  
  <ul>
    <li>
      <a href="#" class="dropdown">Switch Branches (6)</a>
      <ul>
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/blockheaders/net.cpp" action="show">blockheaders</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/log-timestamp/net.cpp" action="show">log-timestamp</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/master/net.cpp" action="show">master</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/portoption/net.cpp" action="show">portoption</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/setaccountfix/net.cpp" action="show">setaccountfix</a></li>
          
        
          
          
            <li><a href="/TheBlueMatt/bitcoin/blob/upnp/net.cpp" action="show">upnp</a></li>
          
        
      </ul>
    </li>
    <li>
      <a href="#" class="dropdown ">Switch Tags (3)</a>
              <ul>
                      
              <li><a href="/TheBlueMatt/bitcoin/blob/v0.3.20.2/net.cpp">v0.3.20.2</a></li>
            
                      
              <li><a href="/TheBlueMatt/bitcoin/blob/v0.3.20/net.cpp">v0.3.20</a></li>
            
                      
              <li><a href="/TheBlueMatt/bitcoin/blob/v0.3.19/net.cpp">v0.3.19</a></li>
            
                  </ul>
      
    </li>
    <li>
    
    <a href="/TheBlueMatt/bitcoin/branches/master" class="manage">Branch List</a>
    
    </li>
  </ul>
</div>

  
  
  
  
  
  



        
    <div class="frame frame-center tree-finder" style="display: none">
      <div class="breadcrumb">
        <b><a href="/TheBlueMatt/bitcoin">bitcoin</a></b> /
        <input class="tree-finder-input" type="text" name="query" autocomplete="off" spellcheck="false">
      </div>

      
        <div class="octotip">
          <p>
            <a href="/TheBlueMatt/bitcoin/dismiss-tree-finder-help" class="dismiss js-dismiss-tree-list-help" title="Hide this notice forever">Dismiss</a>
            <strong>Octotip:</strong> You've activated the <em>file finder</em> by pressing <span class="kbd">t</span>
            Start typing to filter the file list. Use <span class="kbd badmono">↑</span> and <span class="kbd badmono">↓</span> to navigate,
            <span class="kbd">enter</span> to view files.
          </p>
        </div>
      

      <table class="tree-browser" cellpadding="0" cellspacing="0">
        <tr class="js-header"><th>&nbsp;</th><th>name</th></tr>
        <tr class="js-no-results no-results" style="display: none">
          <th colspan="2">No matching files</th>
        </tr>
        <tbody class="js-results-list">
        </tbody>
      </table>
    </div>

    <div id="jump-to-line" style="display:none">
      <h2>Jump to Line</h2>
      <form>
        <input class="textfield" type="text">
        <div class="full-button">
          <button type="submit" class="classy">
            <span>Go</span>
          </button>
        </div>
      </form>
    </div>

    <div id="repo_details" class="metabox clearfix">
      <div id="repo_details_loader" class="metabox-loader" style="display:none">Sending Request&hellip;</div>

        <a href="/TheBlueMatt/bitcoin/downloads" class="download-source" id="download_button" title="Download source, tagged packages and binaries."><span class="icon"></span>Downloads</a>

      <div id="repository_desc_wrapper">
      <div id="repository_description" rel="repository_description_edit">
        
          <p>Bitcoin integration/staging tree
            <span id="read_more" style="display:none">&mdash; <a href="#readme">Read more</a></span>
          </p>
        
      </div>

      <div id="repository_description_edit" style="display:none;" class="inline-edit">
        <form action="/TheBlueMatt/bitcoin/admin/update" method="post"><div style="margin:0;padding:0"><input name="authenticity_token" type="hidden" value="cf1c0b8959733617e26844d243ba9e11c32dfb89" /></div>
          <input type="hidden" name="field" value="repository_description">
          <input type="text" class="textfield" name="value" value="Bitcoin integration/staging tree">
          <div class="form-actions">
            <button class="minibutton"><span>Save</span></button> &nbsp; <a href="#" class="cancel">Cancel</a>
          </div>
        </form>
      </div>

      
      <div class="repository-homepage" id="repository_homepage" rel="repository_homepage_edit">
        <p><a href="http://www.bitcoin.org" rel="nofollow">http://www.bitcoin.org</a></p>
      </div>

      <div id="repository_homepage_edit" style="display:none;" class="inline-edit">
        <form action="/TheBlueMatt/bitcoin/admin/update" method="post"><div style="margin:0;padding:0"><input name="authenticity_token" type="hidden" value="cf1c0b8959733617e26844d243ba9e11c32dfb89" /></div>
          <input type="hidden" name="field" value="repository_homepage">
          <input type="text" class="textfield" name="value" value="http://www.bitcoin.org">
          <div class="form-actions">
            <button class="minibutton"><span>Save</span></button> &nbsp; <a href="#" class="cancel">Cancel</a>
          </div>
        </form>
      </div>
      </div>
      <div class="rule "></div>
            <div id="url_box" class="url-box">
        <ul class="clone-urls">
          
            
              <li id="private_clone_url"><a href="git@github.com:TheBlueMatt/bitcoin.git" data-permissions="Read+Write">SSH</a></li>
            
            <li id="http_clone_url"><a href="https://TheBlueMatt@github.com/TheBlueMatt/bitcoin.git" data-permissions="Read+Write">HTTP</a></li>
            <li id="public_clone_url"><a href="git://github.com/TheBlueMatt/bitcoin.git" data-permissions="Read-Only">Git Read-Only</a></li>
          
          
        </ul>
        <input type="text" spellcheck="false" id="url_field" class="url-field" />
              <span style="display:none" id="url_box_clippy"></span>
      <span id="clippy_tooltip_url_box_clippy" class="clippy-tooltip tooltipped" title="copy to clipboard">
      <object classid="clsid:d27cdb6e-ae6d-11cf-96b8-444553540000"
              width="14"
              height="14"
              class="clippy"
              id="clippy" >
      <param name="movie" value="https://assets2.github.com/flash/clippy.swf?v5"/>
      <param name="allowScriptAccess" value="always" />
      <param name="quality" value="high" />
      <param name="scale" value="noscale" />
      <param NAME="FlashVars" value="id=url_box_clippy&amp;copied=&amp;copyto=">
      <param name="bgcolor" value="#FFFFFF">
      <param name="wmode" value="opaque">
      <embed src="https://assets2.github.com/flash/clippy.swf?v5"
             width="14"
             height="14"
             name="clippy"
             quality="high"
             allowScriptAccess="always"
             type="application/x-shockwave-flash"
             pluginspage="http://www.macromedia.com/go/getflashplayer"
             FlashVars="id=url_box_clippy&amp;copied=&amp;copyto="
             bgcolor="#FFFFFF"
             wmode="opaque"
      />
      </object>
      </span>

        <p id="url_description">This URL has <strong>Read+Write</strong> access</p>
      </div>
    </div>


        

      </div><!-- /.pagehead -->

      

  





<script type="text/javascript">
  GitHub.downloadRepo = '/TheBlueMatt/bitcoin/archives/f0fcb982f7f6a5184df47250b2396d92476c7d99'
  GitHub.revType = "SHA1"

  GitHub.controllerName = "blob"
  GitHub.actionName     = "show"
  GitHub.currentAction  = "blob#show"

  
    GitHub.hasWriteAccess = true
    GitHub.hasAdminAccess = true
    GitHub.watchingRepo = true
    GitHub.ignoredRepo = false
    GitHub.hasForkOfRepo = null
    GitHub.hasForked = false
  

  
</script>






<div class="flash-messages"></div>


  <div id="commit">
    <div class="group">
        
  <div class="envelope commit">
    <div class="human">
      
        <div class="message"><pre><a href="/TheBlueMatt/bitcoin/commit/f0fcb982f7f6a5184df47250b2396d92476c7d99">Be a bit more clear in the build instructions for Unix.</a> </pre></div>
      

      <div class="actor">
        <div class="gravatar">
          
          <img src="https://secure.gravatar.com/avatar/a9d8a2fc99b09023085c134fc4619f2d?s=140&d=https://assets.github.com%2Fimages%2Fgravatars%2Fgravatar-140.png" alt="" width="30" height="30"  />
        </div>
        <div class="name">Matt Corallo <span>(author)</span></div>
        <div class="date">
          <abbr class="relatize" title="2011-03-11 13:57:50">Fri Mar 11 13:57:50 -0800 2011</abbr>
        </div>
      </div>

      

    </div>
    <div class="machine">
      <span>c</span>ommit&nbsp;&nbsp;<a href="/TheBlueMatt/bitcoin/commit/f0fcb982f7f6a5184df47250b2396d92476c7d99" hotkey="c">f0fcb982f7f6a5184df4</a><br />
      <span>t</span>ree&nbsp;&nbsp;&nbsp;&nbsp;<a href="/TheBlueMatt/bitcoin/tree/f0fcb982f7f6a5184df47250b2396d92476c7d99/build-unix.txt" hotkey="t">306da4a709069ad6f4a0</a><br />
      
        <span>p</span>arent&nbsp;
        
        <a href="/TheBlueMatt/bitcoin/commit/fd7e34f46f6c4e3a065e3604ae43e9c0425aad71" hotkey="p">fd7e34f46f6c4e3a065e</a>
      

    </div>
  </div>

    </div>
  </div>



  <div id="slider">

  

    <div class="breadcrumb" data-path="net.cpp/">
      <b><a href="/TheBlueMatt/bitcoin/tree/2505478ab38b297fe4b343372473c12b6f8ee89c">bitcoin</a></b> / net.cpp       <span style="display:none" id="clippy_4071">net.cpp</span>
      
      <object classid="clsid:d27cdb6e-ae6d-11cf-96b8-444553540000"
              width="110"
              height="14"
              class="clippy"
              id="clippy" >
      <param name="movie" value="https://assets2.github.com/flash/clippy.swf?v5"/>
      <param name="allowScriptAccess" value="always" />
      <param name="quality" value="high" />
      <param name="scale" value="noscale" />
      <param NAME="FlashVars" value="id=clippy_4071&amp;copied=copied!&amp;copyto=copy to clipboard">
      <param name="bgcolor" value="#FFFFFF">
      <param name="wmode" value="opaque">
      <embed src="https://assets2.github.com/flash/clippy.swf?v5"
             width="110"
             height="14"
             name="clippy"
             quality="high"
             allowScriptAccess="always"
             type="application/x-shockwave-flash"
             pluginspage="http://www.macromedia.com/go/getflashplayer"
             FlashVars="id=clippy_4071&amp;copied=copied!&amp;copyto=copy to clipboard"
             bgcolor="#FFFFFF"
             wmode="opaque"
      />
      </object>
      

    </div>

    <div class="frames">
      <div class="frame frame-center" data-path="net.cpp/">
        
          <ul class="big-actions">
            
            <li><a class="file-edit-link minibutton" href="/TheBlueMatt/bitcoin/file-edit/__current_ref__/net.cpp"><span>Edit this file</span></a></li>
          </ul>
        

        <div id="files">
          <div class="file">
            <div class="meta">
              <div class="info">
                <span class="icon"><img alt="Txt" height="16" src="https://assets0.github.com/images/icons/txt.png?2b893794092cd44a789db64b3da95d1091187f55" width="16" /></span>
                <span class="mode" title="File Mode">100644</span>
                
                  <span>1563 lines (1338 sloc)</span>
                
                <span>51.306 kb</span>
              </div>
              <ul class="actions">
                <li><a href="/TheBlueMatt/bitcoin/raw/2505478ab38b297fe4b343372473c12b6f8ee89c/net.cpp" id="raw-url">raw</a></li>
                
                  <li><a href="/TheBlueMatt/bitcoin/blame/2505478ab38b297fe4b343372473c12b6f8ee89c/net.cpp">blame</a></li>
                
                <li><a href="/TheBlueMatt/bitcoin/commits/2505478ab38b297fe4b343372473c12b6f8ee89c/net.cpp">history</a></li>
              </ul>
            </div>
            
  <div class="data type-cpp">
    
      <table cellpadding="0" cellspacing="0">
        <tr>
          <td>
            <pre class="line_numbers"><span id="L1" rel="#L1">1</span>
<span id="L2" rel="#L2">2</span>
<span id="L3" rel="#L3">3</span>
<span id="L4" rel="#L4">4</span>
<span id="L5" rel="#L5">5</span>
<span id="L6" rel="#L6">6</span>
<span id="L7" rel="#L7">7</span>
<span id="L8" rel="#L8">8</span>
<span id="L9" rel="#L9">9</span>
<span id="L10" rel="#L10">10</span>
<span id="L11" rel="#L11">11</span>
<span id="L12" rel="#L12">12</span>
<span id="L13" rel="#L13">13</span>
<span id="L14" rel="#L14">14</span>
<span id="L15" rel="#L15">15</span>
<span id="L16" rel="#L16">16</span>
<span id="L17" rel="#L17">17</span>
<span id="L18" rel="#L18">18</span>
<span id="L19" rel="#L19">19</span>
<span id="L20" rel="#L20">20</span>
<span id="L21" rel="#L21">21</span>
<span id="L22" rel="#L22">22</span>
<span id="L23" rel="#L23">23</span>
<span id="L24" rel="#L24">24</span>
<span id="L25" rel="#L25">25</span>
<span id="L26" rel="#L26">26</span>
<span id="L27" rel="#L27">27</span>
<span id="L28" rel="#L28">28</span>
<span id="L29" rel="#L29">29</span>
<span id="L30" rel="#L30">30</span>
<span id="L31" rel="#L31">31</span>
<span id="L32" rel="#L32">32</span>
<span id="L33" rel="#L33">33</span>
<span id="L34" rel="#L34">34</span>
<span id="L35" rel="#L35">35</span>
<span id="L36" rel="#L36">36</span>
<span id="L37" rel="#L37">37</span>
<span id="L38" rel="#L38">38</span>
<span id="L39" rel="#L39">39</span>
<span id="L40" rel="#L40">40</span>
<span id="L41" rel="#L41">41</span>
<span id="L42" rel="#L42">42</span>
<span id="L43" rel="#L43">43</span>
<span id="L44" rel="#L44">44</span>
<span id="L45" rel="#L45">45</span>
<span id="L46" rel="#L46">46</span>
<span id="L47" rel="#L47">47</span>
<span id="L48" rel="#L48">48</span>
<span id="L49" rel="#L49">49</span>
<span id="L50" rel="#L50">50</span>
<span id="L51" rel="#L51">51</span>
<span id="L52" rel="#L52">52</span>
<span id="L53" rel="#L53">53</span>
<span id="L54" rel="#L54">54</span>
<span id="L55" rel="#L55">55</span>
<span id="L56" rel="#L56">56</span>
<span id="L57" rel="#L57">57</span>
<span id="L58" rel="#L58">58</span>
<span id="L59" rel="#L59">59</span>
<span id="L60" rel="#L60">60</span>
<span id="L61" rel="#L61">61</span>
<span id="L62" rel="#L62">62</span>
<span id="L63" rel="#L63">63</span>
<span id="L64" rel="#L64">64</span>
<span id="L65" rel="#L65">65</span>
<span id="L66" rel="#L66">66</span>
<span id="L67" rel="#L67">67</span>
<span id="L68" rel="#L68">68</span>
<span id="L69" rel="#L69">69</span>
<span id="L70" rel="#L70">70</span>
<span id="L71" rel="#L71">71</span>
<span id="L72" rel="#L72">72</span>
<span id="L73" rel="#L73">73</span>
<span id="L74" rel="#L74">74</span>
<span id="L75" rel="#L75">75</span>
<span id="L76" rel="#L76">76</span>
<span id="L77" rel="#L77">77</span>
<span id="L78" rel="#L78">78</span>
<span id="L79" rel="#L79">79</span>
<span id="L80" rel="#L80">80</span>
<span id="L81" rel="#L81">81</span>
<span id="L82" rel="#L82">82</span>
<span id="L83" rel="#L83">83</span>
<span id="L84" rel="#L84">84</span>
<span id="L85" rel="#L85">85</span>
<span id="L86" rel="#L86">86</span>
<span id="L87" rel="#L87">87</span>
<span id="L88" rel="#L88">88</span>
<span id="L89" rel="#L89">89</span>
<span id="L90" rel="#L90">90</span>
<span id="L91" rel="#L91">91</span>
<span id="L92" rel="#L92">92</span>
<span id="L93" rel="#L93">93</span>
<span id="L94" rel="#L94">94</span>
<span id="L95" rel="#L95">95</span>
<span id="L96" rel="#L96">96</span>
<span id="L97" rel="#L97">97</span>
<span id="L98" rel="#L98">98</span>
<span id="L99" rel="#L99">99</span>
<span id="L100" rel="#L100">100</span>
<span id="L101" rel="#L101">101</span>
<span id="L102" rel="#L102">102</span>
<span id="L103" rel="#L103">103</span>
<span id="L104" rel="#L104">104</span>
<span id="L105" rel="#L105">105</span>
<span id="L106" rel="#L106">106</span>
<span id="L107" rel="#L107">107</span>
<span id="L108" rel="#L108">108</span>
<span id="L109" rel="#L109">109</span>
<span id="L110" rel="#L110">110</span>
<span id="L111" rel="#L111">111</span>
<span id="L112" rel="#L112">112</span>
<span id="L113" rel="#L113">113</span>
<span id="L114" rel="#L114">114</span>
<span id="L115" rel="#L115">115</span>
<span id="L116" rel="#L116">116</span>
<span id="L117" rel="#L117">117</span>
<span id="L118" rel="#L118">118</span>
<span id="L119" rel="#L119">119</span>
<span id="L120" rel="#L120">120</span>
<span id="L121" rel="#L121">121</span>
<span id="L122" rel="#L122">122</span>
<span id="L123" rel="#L123">123</span>
<span id="L124" rel="#L124">124</span>
<span id="L125" rel="#L125">125</span>
<span id="L126" rel="#L126">126</span>
<span id="L127" rel="#L127">127</span>
<span id="L128" rel="#L128">128</span>
<span id="L129" rel="#L129">129</span>
<span id="L130" rel="#L130">130</span>
<span id="L131" rel="#L131">131</span>
<span id="L132" rel="#L132">132</span>
<span id="L133" rel="#L133">133</span>
<span id="L134" rel="#L134">134</span>
<span id="L135" rel="#L135">135</span>
<span id="L136" rel="#L136">136</span>
<span id="L137" rel="#L137">137</span>
<span id="L138" rel="#L138">138</span>
<span id="L139" rel="#L139">139</span>
<span id="L140" rel="#L140">140</span>
<span id="L141" rel="#L141">141</span>
<span id="L142" rel="#L142">142</span>
<span id="L143" rel="#L143">143</span>
<span id="L144" rel="#L144">144</span>
<span id="L145" rel="#L145">145</span>
<span id="L146" rel="#L146">146</span>
<span id="L147" rel="#L147">147</span>
<span id="L148" rel="#L148">148</span>
<span id="L149" rel="#L149">149</span>
<span id="L150" rel="#L150">150</span>
<span id="L151" rel="#L151">151</span>
<span id="L152" rel="#L152">152</span>
<span id="L153" rel="#L153">153</span>
<span id="L154" rel="#L154">154</span>
<span id="L155" rel="#L155">155</span>
<span id="L156" rel="#L156">156</span>
<span id="L157" rel="#L157">157</span>
<span id="L158" rel="#L158">158</span>
<span id="L159" rel="#L159">159</span>
<span id="L160" rel="#L160">160</span>
<span id="L161" rel="#L161">161</span>
<span id="L162" rel="#L162">162</span>
<span id="L163" rel="#L163">163</span>
<span id="L164" rel="#L164">164</span>
<span id="L165" rel="#L165">165</span>
<span id="L166" rel="#L166">166</span>
<span id="L167" rel="#L167">167</span>
<span id="L168" rel="#L168">168</span>
<span id="L169" rel="#L169">169</span>
<span id="L170" rel="#L170">170</span>
<span id="L171" rel="#L171">171</span>
<span id="L172" rel="#L172">172</span>
<span id="L173" rel="#L173">173</span>
<span id="L174" rel="#L174">174</span>
<span id="L175" rel="#L175">175</span>
<span id="L176" rel="#L176">176</span>
<span id="L177" rel="#L177">177</span>
<span id="L178" rel="#L178">178</span>
<span id="L179" rel="#L179">179</span>
<span id="L180" rel="#L180">180</span>
<span id="L181" rel="#L181">181</span>
<span id="L182" rel="#L182">182</span>
<span id="L183" rel="#L183">183</span>
<span id="L184" rel="#L184">184</span>
<span id="L185" rel="#L185">185</span>
<span id="L186" rel="#L186">186</span>
<span id="L187" rel="#L187">187</span>
<span id="L188" rel="#L188">188</span>
<span id="L189" rel="#L189">189</span>
<span id="L190" rel="#L190">190</span>
<span id="L191" rel="#L191">191</span>
<span id="L192" rel="#L192">192</span>
<span id="L193" rel="#L193">193</span>
<span id="L194" rel="#L194">194</span>
<span id="L195" rel="#L195">195</span>
<span id="L196" rel="#L196">196</span>
<span id="L197" rel="#L197">197</span>
<span id="L198" rel="#L198">198</span>
<span id="L199" rel="#L199">199</span>
<span id="L200" rel="#L200">200</span>
<span id="L201" rel="#L201">201</span>
<span id="L202" rel="#L202">202</span>
<span id="L203" rel="#L203">203</span>
<span id="L204" rel="#L204">204</span>
<span id="L205" rel="#L205">205</span>
<span id="L206" rel="#L206">206</span>
<span id="L207" rel="#L207">207</span>
<span id="L208" rel="#L208">208</span>
<span id="L209" rel="#L209">209</span>
<span id="L210" rel="#L210">210</span>
<span id="L211" rel="#L211">211</span>
<span id="L212" rel="#L212">212</span>
<span id="L213" rel="#L213">213</span>
<span id="L214" rel="#L214">214</span>
<span id="L215" rel="#L215">215</span>
<span id="L216" rel="#L216">216</span>
<span id="L217" rel="#L217">217</span>
<span id="L218" rel="#L218">218</span>
<span id="L219" rel="#L219">219</span>
<span id="L220" rel="#L220">220</span>
<span id="L221" rel="#L221">221</span>
<span id="L222" rel="#L222">222</span>
<span id="L223" rel="#L223">223</span>
<span id="L224" rel="#L224">224</span>
<span id="L225" rel="#L225">225</span>
<span id="L226" rel="#L226">226</span>
<span id="L227" rel="#L227">227</span>
<span id="L228" rel="#L228">228</span>
<span id="L229" rel="#L229">229</span>
<span id="L230" rel="#L230">230</span>
<span id="L231" rel="#L231">231</span>
<span id="L232" rel="#L232">232</span>
<span id="L233" rel="#L233">233</span>
<span id="L234" rel="#L234">234</span>
<span id="L235" rel="#L235">235</span>
<span id="L236" rel="#L236">236</span>
<span id="L237" rel="#L237">237</span>
<span id="L238" rel="#L238">238</span>
<span id="L239" rel="#L239">239</span>
<span id="L240" rel="#L240">240</span>
<span id="L241" rel="#L241">241</span>
<span id="L242" rel="#L242">242</span>
<span id="L243" rel="#L243">243</span>
<span id="L244" rel="#L244">244</span>
<span id="L245" rel="#L245">245</span>
<span id="L246" rel="#L246">246</span>
<span id="L247" rel="#L247">247</span>
<span id="L248" rel="#L248">248</span>
<span id="L249" rel="#L249">249</span>
<span id="L250" rel="#L250">250</span>
<span id="L251" rel="#L251">251</span>
<span id="L252" rel="#L252">252</span>
<span id="L253" rel="#L253">253</span>
<span id="L254" rel="#L254">254</span>
<span id="L255" rel="#L255">255</span>
<span id="L256" rel="#L256">256</span>
<span id="L257" rel="#L257">257</span>
<span id="L258" rel="#L258">258</span>
<span id="L259" rel="#L259">259</span>
<span id="L260" rel="#L260">260</span>
<span id="L261" rel="#L261">261</span>
<span id="L262" rel="#L262">262</span>
<span id="L263" rel="#L263">263</span>
<span id="L264" rel="#L264">264</span>
<span id="L265" rel="#L265">265</span>
<span id="L266" rel="#L266">266</span>
<span id="L267" rel="#L267">267</span>
<span id="L268" rel="#L268">268</span>
<span id="L269" rel="#L269">269</span>
<span id="L270" rel="#L270">270</span>
<span id="L271" rel="#L271">271</span>
<span id="L272" rel="#L272">272</span>
<span id="L273" rel="#L273">273</span>
<span id="L274" rel="#L274">274</span>
<span id="L275" rel="#L275">275</span>
<span id="L276" rel="#L276">276</span>
<span id="L277" rel="#L277">277</span>
<span id="L278" rel="#L278">278</span>
<span id="L279" rel="#L279">279</span>
<span id="L280" rel="#L280">280</span>
<span id="L281" rel="#L281">281</span>
<span id="L282" rel="#L282">282</span>
<span id="L283" rel="#L283">283</span>
<span id="L284" rel="#L284">284</span>
<span id="L285" rel="#L285">285</span>
<span id="L286" rel="#L286">286</span>
<span id="L287" rel="#L287">287</span>
<span id="L288" rel="#L288">288</span>
<span id="L289" rel="#L289">289</span>
<span id="L290" rel="#L290">290</span>
<span id="L291" rel="#L291">291</span>
<span id="L292" rel="#L292">292</span>
<span id="L293" rel="#L293">293</span>
<span id="L294" rel="#L294">294</span>
<span id="L295" rel="#L295">295</span>
<span id="L296" rel="#L296">296</span>
<span id="L297" rel="#L297">297</span>
<span id="L298" rel="#L298">298</span>
<span id="L299" rel="#L299">299</span>
<span id="L300" rel="#L300">300</span>
<span id="L301" rel="#L301">301</span>
<span id="L302" rel="#L302">302</span>
<span id="L303" rel="#L303">303</span>
<span id="L304" rel="#L304">304</span>
<span id="L305" rel="#L305">305</span>
<span id="L306" rel="#L306">306</span>
<span id="L307" rel="#L307">307</span>
<span id="L308" rel="#L308">308</span>
<span id="L309" rel="#L309">309</span>
<span id="L310" rel="#L310">310</span>
<span id="L311" rel="#L311">311</span>
<span id="L312" rel="#L312">312</span>
<span id="L313" rel="#L313">313</span>
<span id="L314" rel="#L314">314</span>
<span id="L315" rel="#L315">315</span>
<span id="L316" rel="#L316">316</span>
<span id="L317" rel="#L317">317</span>
<span id="L318" rel="#L318">318</span>
<span id="L319" rel="#L319">319</span>
<span id="L320" rel="#L320">320</span>
<span id="L321" rel="#L321">321</span>
<span id="L322" rel="#L322">322</span>
<span id="L323" rel="#L323">323</span>
<span id="L324" rel="#L324">324</span>
<span id="L325" rel="#L325">325</span>
<span id="L326" rel="#L326">326</span>
<span id="L327" rel="#L327">327</span>
<span id="L328" rel="#L328">328</span>
<span id="L329" rel="#L329">329</span>
<span id="L330" rel="#L330">330</span>
<span id="L331" rel="#L331">331</span>
<span id="L332" rel="#L332">332</span>
<span id="L333" rel="#L333">333</span>
<span id="L334" rel="#L334">334</span>
<span id="L335" rel="#L335">335</span>
<span id="L336" rel="#L336">336</span>
<span id="L337" rel="#L337">337</span>
<span id="L338" rel="#L338">338</span>
<span id="L339" rel="#L339">339</span>
<span id="L340" rel="#L340">340</span>
<span id="L341" rel="#L341">341</span>
<span id="L342" rel="#L342">342</span>
<span id="L343" rel="#L343">343</span>
<span id="L344" rel="#L344">344</span>
<span id="L345" rel="#L345">345</span>
<span id="L346" rel="#L346">346</span>
<span id="L347" rel="#L347">347</span>
<span id="L348" rel="#L348">348</span>
<span id="L349" rel="#L349">349</span>
<span id="L350" rel="#L350">350</span>
<span id="L351" rel="#L351">351</span>
<span id="L352" rel="#L352">352</span>
<span id="L353" rel="#L353">353</span>
<span id="L354" rel="#L354">354</span>
<span id="L355" rel="#L355">355</span>
<span id="L356" rel="#L356">356</span>
<span id="L357" rel="#L357">357</span>
<span id="L358" rel="#L358">358</span>
<span id="L359" rel="#L359">359</span>
<span id="L360" rel="#L360">360</span>
<span id="L361" rel="#L361">361</span>
<span id="L362" rel="#L362">362</span>
<span id="L363" rel="#L363">363</span>
<span id="L364" rel="#L364">364</span>
<span id="L365" rel="#L365">365</span>
<span id="L366" rel="#L366">366</span>
<span id="L367" rel="#L367">367</span>
<span id="L368" rel="#L368">368</span>
<span id="L369" rel="#L369">369</span>
<span id="L370" rel="#L370">370</span>
<span id="L371" rel="#L371">371</span>
<span id="L372" rel="#L372">372</span>
<span id="L373" rel="#L373">373</span>
<span id="L374" rel="#L374">374</span>
<span id="L375" rel="#L375">375</span>
<span id="L376" rel="#L376">376</span>
<span id="L377" rel="#L377">377</span>
<span id="L378" rel="#L378">378</span>
<span id="L379" rel="#L379">379</span>
<span id="L380" rel="#L380">380</span>
<span id="L381" rel="#L381">381</span>
<span id="L382" rel="#L382">382</span>
<span id="L383" rel="#L383">383</span>
<span id="L384" rel="#L384">384</span>
<span id="L385" rel="#L385">385</span>
<span id="L386" rel="#L386">386</span>
<span id="L387" rel="#L387">387</span>
<span id="L388" rel="#L388">388</span>
<span id="L389" rel="#L389">389</span>
<span id="L390" rel="#L390">390</span>
<span id="L391" rel="#L391">391</span>
<span id="L392" rel="#L392">392</span>
<span id="L393" rel="#L393">393</span>
<span id="L394" rel="#L394">394</span>
<span id="L395" rel="#L395">395</span>
<span id="L396" rel="#L396">396</span>
<span id="L397" rel="#L397">397</span>
<span id="L398" rel="#L398">398</span>
<span id="L399" rel="#L399">399</span>
<span id="L400" rel="#L400">400</span>
<span id="L401" rel="#L401">401</span>
<span id="L402" rel="#L402">402</span>
<span id="L403" rel="#L403">403</span>
<span id="L404" rel="#L404">404</span>
<span id="L405" rel="#L405">405</span>
<span id="L406" rel="#L406">406</span>
<span id="L407" rel="#L407">407</span>
<span id="L408" rel="#L408">408</span>
<span id="L409" rel="#L409">409</span>
<span id="L410" rel="#L410">410</span>
<span id="L411" rel="#L411">411</span>
<span id="L412" rel="#L412">412</span>
<span id="L413" rel="#L413">413</span>
<span id="L414" rel="#L414">414</span>
<span id="L415" rel="#L415">415</span>
<span id="L416" rel="#L416">416</span>
<span id="L417" rel="#L417">417</span>
<span id="L418" rel="#L418">418</span>
<span id="L419" rel="#L419">419</span>
<span id="L420" rel="#L420">420</span>
<span id="L421" rel="#L421">421</span>
<span id="L422" rel="#L422">422</span>
<span id="L423" rel="#L423">423</span>
<span id="L424" rel="#L424">424</span>
<span id="L425" rel="#L425">425</span>
<span id="L426" rel="#L426">426</span>
<span id="L427" rel="#L427">427</span>
<span id="L428" rel="#L428">428</span>
<span id="L429" rel="#L429">429</span>
<span id="L430" rel="#L430">430</span>
<span id="L431" rel="#L431">431</span>
<span id="L432" rel="#L432">432</span>
<span id="L433" rel="#L433">433</span>
<span id="L434" rel="#L434">434</span>
<span id="L435" rel="#L435">435</span>
<span id="L436" rel="#L436">436</span>
<span id="L437" rel="#L437">437</span>
<span id="L438" rel="#L438">438</span>
<span id="L439" rel="#L439">439</span>
<span id="L440" rel="#L440">440</span>
<span id="L441" rel="#L441">441</span>
<span id="L442" rel="#L442">442</span>
<span id="L443" rel="#L443">443</span>
<span id="L444" rel="#L444">444</span>
<span id="L445" rel="#L445">445</span>
<span id="L446" rel="#L446">446</span>
<span id="L447" rel="#L447">447</span>
<span id="L448" rel="#L448">448</span>
<span id="L449" rel="#L449">449</span>
<span id="L450" rel="#L450">450</span>
<span id="L451" rel="#L451">451</span>
<span id="L452" rel="#L452">452</span>
<span id="L453" rel="#L453">453</span>
<span id="L454" rel="#L454">454</span>
<span id="L455" rel="#L455">455</span>
<span id="L456" rel="#L456">456</span>
<span id="L457" rel="#L457">457</span>
<span id="L458" rel="#L458">458</span>
<span id="L459" rel="#L459">459</span>
<span id="L460" rel="#L460">460</span>
<span id="L461" rel="#L461">461</span>
<span id="L462" rel="#L462">462</span>
<span id="L463" rel="#L463">463</span>
<span id="L464" rel="#L464">464</span>
<span id="L465" rel="#L465">465</span>
<span id="L466" rel="#L466">466</span>
<span id="L467" rel="#L467">467</span>
<span id="L468" rel="#L468">468</span>
<span id="L469" rel="#L469">469</span>
<span id="L470" rel="#L470">470</span>
<span id="L471" rel="#L471">471</span>
<span id="L472" rel="#L472">472</span>
<span id="L473" rel="#L473">473</span>
<span id="L474" rel="#L474">474</span>
<span id="L475" rel="#L475">475</span>
<span id="L476" rel="#L476">476</span>
<span id="L477" rel="#L477">477</span>
<span id="L478" rel="#L478">478</span>
<span id="L479" rel="#L479">479</span>
<span id="L480" rel="#L480">480</span>
<span id="L481" rel="#L481">481</span>
<span id="L482" rel="#L482">482</span>
<span id="L483" rel="#L483">483</span>
<span id="L484" rel="#L484">484</span>
<span id="L485" rel="#L485">485</span>
<span id="L486" rel="#L486">486</span>
<span id="L487" rel="#L487">487</span>
<span id="L488" rel="#L488">488</span>
<span id="L489" rel="#L489">489</span>
<span id="L490" rel="#L490">490</span>
<span id="L491" rel="#L491">491</span>
<span id="L492" rel="#L492">492</span>
<span id="L493" rel="#L493">493</span>
<span id="L494" rel="#L494">494</span>
<span id="L495" rel="#L495">495</span>
<span id="L496" rel="#L496">496</span>
<span id="L497" rel="#L497">497</span>
<span id="L498" rel="#L498">498</span>
<span id="L499" rel="#L499">499</span>
<span id="L500" rel="#L500">500</span>
<span id="L501" rel="#L501">501</span>
<span id="L502" rel="#L502">502</span>
<span id="L503" rel="#L503">503</span>
<span id="L504" rel="#L504">504</span>
<span id="L505" rel="#L505">505</span>
<span id="L506" rel="#L506">506</span>
<span id="L507" rel="#L507">507</span>
<span id="L508" rel="#L508">508</span>
<span id="L509" rel="#L509">509</span>
<span id="L510" rel="#L510">510</span>
<span id="L511" rel="#L511">511</span>
<span id="L512" rel="#L512">512</span>
<span id="L513" rel="#L513">513</span>
<span id="L514" rel="#L514">514</span>
<span id="L515" rel="#L515">515</span>
<span id="L516" rel="#L516">516</span>
<span id="L517" rel="#L517">517</span>
<span id="L518" rel="#L518">518</span>
<span id="L519" rel="#L519">519</span>
<span id="L520" rel="#L520">520</span>
<span id="L521" rel="#L521">521</span>
<span id="L522" rel="#L522">522</span>
<span id="L523" rel="#L523">523</span>
<span id="L524" rel="#L524">524</span>
<span id="L525" rel="#L525">525</span>
<span id="L526" rel="#L526">526</span>
<span id="L527" rel="#L527">527</span>
<span id="L528" rel="#L528">528</span>
<span id="L529" rel="#L529">529</span>
<span id="L530" rel="#L530">530</span>
<span id="L531" rel="#L531">531</span>
<span id="L532" rel="#L532">532</span>
<span id="L533" rel="#L533">533</span>
<span id="L534" rel="#L534">534</span>
<span id="L535" rel="#L535">535</span>
<span id="L536" rel="#L536">536</span>
<span id="L537" rel="#L537">537</span>
<span id="L538" rel="#L538">538</span>
<span id="L539" rel="#L539">539</span>
<span id="L540" rel="#L540">540</span>
<span id="L541" rel="#L541">541</span>
<span id="L542" rel="#L542">542</span>
<span id="L543" rel="#L543">543</span>
<span id="L544" rel="#L544">544</span>
<span id="L545" rel="#L545">545</span>
<span id="L546" rel="#L546">546</span>
<span id="L547" rel="#L547">547</span>
<span id="L548" rel="#L548">548</span>
<span id="L549" rel="#L549">549</span>
<span id="L550" rel="#L550">550</span>
<span id="L551" rel="#L551">551</span>
<span id="L552" rel="#L552">552</span>
<span id="L553" rel="#L553">553</span>
<span id="L554" rel="#L554">554</span>
<span id="L555" rel="#L555">555</span>
<span id="L556" rel="#L556">556</span>
<span id="L557" rel="#L557">557</span>
<span id="L558" rel="#L558">558</span>
<span id="L559" rel="#L559">559</span>
<span id="L560" rel="#L560">560</span>
<span id="L561" rel="#L561">561</span>
<span id="L562" rel="#L562">562</span>
<span id="L563" rel="#L563">563</span>
<span id="L564" rel="#L564">564</span>
<span id="L565" rel="#L565">565</span>
<span id="L566" rel="#L566">566</span>
<span id="L567" rel="#L567">567</span>
<span id="L568" rel="#L568">568</span>
<span id="L569" rel="#L569">569</span>
<span id="L570" rel="#L570">570</span>
<span id="L571" rel="#L571">571</span>
<span id="L572" rel="#L572">572</span>
<span id="L573" rel="#L573">573</span>
<span id="L574" rel="#L574">574</span>
<span id="L575" rel="#L575">575</span>
<span id="L576" rel="#L576">576</span>
<span id="L577" rel="#L577">577</span>
<span id="L578" rel="#L578">578</span>
<span id="L579" rel="#L579">579</span>
<span id="L580" rel="#L580">580</span>
<span id="L581" rel="#L581">581</span>
<span id="L582" rel="#L582">582</span>
<span id="L583" rel="#L583">583</span>
<span id="L584" rel="#L584">584</span>
<span id="L585" rel="#L585">585</span>
<span id="L586" rel="#L586">586</span>
<span id="L587" rel="#L587">587</span>
<span id="L588" rel="#L588">588</span>
<span id="L589" rel="#L589">589</span>
<span id="L590" rel="#L590">590</span>
<span id="L591" rel="#L591">591</span>
<span id="L592" rel="#L592">592</span>
<span id="L593" rel="#L593">593</span>
<span id="L594" rel="#L594">594</span>
<span id="L595" rel="#L595">595</span>
<span id="L596" rel="#L596">596</span>
<span id="L597" rel="#L597">597</span>
<span id="L598" rel="#L598">598</span>
<span id="L599" rel="#L599">599</span>
<span id="L600" rel="#L600">600</span>
<span id="L601" rel="#L601">601</span>
<span id="L602" rel="#L602">602</span>
<span id="L603" rel="#L603">603</span>
<span id="L604" rel="#L604">604</span>
<span id="L605" rel="#L605">605</span>
<span id="L606" rel="#L606">606</span>
<span id="L607" rel="#L607">607</span>
<span id="L608" rel="#L608">608</span>
<span id="L609" rel="#L609">609</span>
<span id="L610" rel="#L610">610</span>
<span id="L611" rel="#L611">611</span>
<span id="L612" rel="#L612">612</span>
<span id="L613" rel="#L613">613</span>
<span id="L614" rel="#L614">614</span>
<span id="L615" rel="#L615">615</span>
<span id="L616" rel="#L616">616</span>
<span id="L617" rel="#L617">617</span>
<span id="L618" rel="#L618">618</span>
<span id="L619" rel="#L619">619</span>
<span id="L620" rel="#L620">620</span>
<span id="L621" rel="#L621">621</span>
<span id="L622" rel="#L622">622</span>
<span id="L623" rel="#L623">623</span>
<span id="L624" rel="#L624">624</span>
<span id="L625" rel="#L625">625</span>
<span id="L626" rel="#L626">626</span>
<span id="L627" rel="#L627">627</span>
<span id="L628" rel="#L628">628</span>
<span id="L629" rel="#L629">629</span>
<span id="L630" rel="#L630">630</span>
<span id="L631" rel="#L631">631</span>
<span id="L632" rel="#L632">632</span>
<span id="L633" rel="#L633">633</span>
<span id="L634" rel="#L634">634</span>
<span id="L635" rel="#L635">635</span>
<span id="L636" rel="#L636">636</span>
<span id="L637" rel="#L637">637</span>
<span id="L638" rel="#L638">638</span>
<span id="L639" rel="#L639">639</span>
<span id="L640" rel="#L640">640</span>
<span id="L641" rel="#L641">641</span>
<span id="L642" rel="#L642">642</span>
<span id="L643" rel="#L643">643</span>
<span id="L644" rel="#L644">644</span>
<span id="L645" rel="#L645">645</span>
<span id="L646" rel="#L646">646</span>
<span id="L647" rel="#L647">647</span>
<span id="L648" rel="#L648">648</span>
<span id="L649" rel="#L649">649</span>
<span id="L650" rel="#L650">650</span>
<span id="L651" rel="#L651">651</span>
<span id="L652" rel="#L652">652</span>
<span id="L653" rel="#L653">653</span>
<span id="L654" rel="#L654">654</span>
<span id="L655" rel="#L655">655</span>
<span id="L656" rel="#L656">656</span>
<span id="L657" rel="#L657">657</span>
<span id="L658" rel="#L658">658</span>
<span id="L659" rel="#L659">659</span>
<span id="L660" rel="#L660">660</span>
<span id="L661" rel="#L661">661</span>
<span id="L662" rel="#L662">662</span>
<span id="L663" rel="#L663">663</span>
<span id="L664" rel="#L664">664</span>
<span id="L665" rel="#L665">665</span>
<span id="L666" rel="#L666">666</span>
<span id="L667" rel="#L667">667</span>
<span id="L668" rel="#L668">668</span>
<span id="L669" rel="#L669">669</span>
<span id="L670" rel="#L670">670</span>
<span id="L671" rel="#L671">671</span>
<span id="L672" rel="#L672">672</span>
<span id="L673" rel="#L673">673</span>
<span id="L674" rel="#L674">674</span>
<span id="L675" rel="#L675">675</span>
<span id="L676" rel="#L676">676</span>
<span id="L677" rel="#L677">677</span>
<span id="L678" rel="#L678">678</span>
<span id="L679" rel="#L679">679</span>
<span id="L680" rel="#L680">680</span>
<span id="L681" rel="#L681">681</span>
<span id="L682" rel="#L682">682</span>
<span id="L683" rel="#L683">683</span>
<span id="L684" rel="#L684">684</span>
<span id="L685" rel="#L685">685</span>
<span id="L686" rel="#L686">686</span>
<span id="L687" rel="#L687">687</span>
<span id="L688" rel="#L688">688</span>
<span id="L689" rel="#L689">689</span>
<span id="L690" rel="#L690">690</span>
<span id="L691" rel="#L691">691</span>
<span id="L692" rel="#L692">692</span>
<span id="L693" rel="#L693">693</span>
<span id="L694" rel="#L694">694</span>
<span id="L695" rel="#L695">695</span>
<span id="L696" rel="#L696">696</span>
<span id="L697" rel="#L697">697</span>
<span id="L698" rel="#L698">698</span>
<span id="L699" rel="#L699">699</span>
<span id="L700" rel="#L700">700</span>
<span id="L701" rel="#L701">701</span>
<span id="L702" rel="#L702">702</span>
<span id="L703" rel="#L703">703</span>
<span id="L704" rel="#L704">704</span>
<span id="L705" rel="#L705">705</span>
<span id="L706" rel="#L706">706</span>
<span id="L707" rel="#L707">707</span>
<span id="L708" rel="#L708">708</span>
<span id="L709" rel="#L709">709</span>
<span id="L710" rel="#L710">710</span>
<span id="L711" rel="#L711">711</span>
<span id="L712" rel="#L712">712</span>
<span id="L713" rel="#L713">713</span>
<span id="L714" rel="#L714">714</span>
<span id="L715" rel="#L715">715</span>
<span id="L716" rel="#L716">716</span>
<span id="L717" rel="#L717">717</span>
<span id="L718" rel="#L718">718</span>
<span id="L719" rel="#L719">719</span>
<span id="L720" rel="#L720">720</span>
<span id="L721" rel="#L721">721</span>
<span id="L722" rel="#L722">722</span>
<span id="L723" rel="#L723">723</span>
<span id="L724" rel="#L724">724</span>
<span id="L725" rel="#L725">725</span>
<span id="L726" rel="#L726">726</span>
<span id="L727" rel="#L727">727</span>
<span id="L728" rel="#L728">728</span>
<span id="L729" rel="#L729">729</span>
<span id="L730" rel="#L730">730</span>
<span id="L731" rel="#L731">731</span>
<span id="L732" rel="#L732">732</span>
<span id="L733" rel="#L733">733</span>
<span id="L734" rel="#L734">734</span>
<span id="L735" rel="#L735">735</span>
<span id="L736" rel="#L736">736</span>
<span id="L737" rel="#L737">737</span>
<span id="L738" rel="#L738">738</span>
<span id="L739" rel="#L739">739</span>
<span id="L740" rel="#L740">740</span>
<span id="L741" rel="#L741">741</span>
<span id="L742" rel="#L742">742</span>
<span id="L743" rel="#L743">743</span>
<span id="L744" rel="#L744">744</span>
<span id="L745" rel="#L745">745</span>
<span id="L746" rel="#L746">746</span>
<span id="L747" rel="#L747">747</span>
<span id="L748" rel="#L748">748</span>
<span id="L749" rel="#L749">749</span>
<span id="L750" rel="#L750">750</span>
<span id="L751" rel="#L751">751</span>
<span id="L752" rel="#L752">752</span>
<span id="L753" rel="#L753">753</span>
<span id="L754" rel="#L754">754</span>
<span id="L755" rel="#L755">755</span>
<span id="L756" rel="#L756">756</span>
<span id="L757" rel="#L757">757</span>
<span id="L758" rel="#L758">758</span>
<span id="L759" rel="#L759">759</span>
<span id="L760" rel="#L760">760</span>
<span id="L761" rel="#L761">761</span>
<span id="L762" rel="#L762">762</span>
<span id="L763" rel="#L763">763</span>
<span id="L764" rel="#L764">764</span>
<span id="L765" rel="#L765">765</span>
<span id="L766" rel="#L766">766</span>
<span id="L767" rel="#L767">767</span>
<span id="L768" rel="#L768">768</span>
<span id="L769" rel="#L769">769</span>
<span id="L770" rel="#L770">770</span>
<span id="L771" rel="#L771">771</span>
<span id="L772" rel="#L772">772</span>
<span id="L773" rel="#L773">773</span>
<span id="L774" rel="#L774">774</span>
<span id="L775" rel="#L775">775</span>
<span id="L776" rel="#L776">776</span>
<span id="L777" rel="#L777">777</span>
<span id="L778" rel="#L778">778</span>
<span id="L779" rel="#L779">779</span>
<span id="L780" rel="#L780">780</span>
<span id="L781" rel="#L781">781</span>
<span id="L782" rel="#L782">782</span>
<span id="L783" rel="#L783">783</span>
<span id="L784" rel="#L784">784</span>
<span id="L785" rel="#L785">785</span>
<span id="L786" rel="#L786">786</span>
<span id="L787" rel="#L787">787</span>
<span id="L788" rel="#L788">788</span>
<span id="L789" rel="#L789">789</span>
<span id="L790" rel="#L790">790</span>
<span id="L791" rel="#L791">791</span>
<span id="L792" rel="#L792">792</span>
<span id="L793" rel="#L793">793</span>
<span id="L794" rel="#L794">794</span>
<span id="L795" rel="#L795">795</span>
<span id="L796" rel="#L796">796</span>
<span id="L797" rel="#L797">797</span>
<span id="L798" rel="#L798">798</span>
<span id="L799" rel="#L799">799</span>
<span id="L800" rel="#L800">800</span>
<span id="L801" rel="#L801">801</span>
<span id="L802" rel="#L802">802</span>
<span id="L803" rel="#L803">803</span>
<span id="L804" rel="#L804">804</span>
<span id="L805" rel="#L805">805</span>
<span id="L806" rel="#L806">806</span>
<span id="L807" rel="#L807">807</span>
<span id="L808" rel="#L808">808</span>
<span id="L809" rel="#L809">809</span>
<span id="L810" rel="#L810">810</span>
<span id="L811" rel="#L811">811</span>
<span id="L812" rel="#L812">812</span>
<span id="L813" rel="#L813">813</span>
<span id="L814" rel="#L814">814</span>
<span id="L815" rel="#L815">815</span>
<span id="L816" rel="#L816">816</span>
<span id="L817" rel="#L817">817</span>
<span id="L818" rel="#L818">818</span>
<span id="L819" rel="#L819">819</span>
<span id="L820" rel="#L820">820</span>
<span id="L821" rel="#L821">821</span>
<span id="L822" rel="#L822">822</span>
<span id="L823" rel="#L823">823</span>
<span id="L824" rel="#L824">824</span>
<span id="L825" rel="#L825">825</span>
<span id="L826" rel="#L826">826</span>
<span id="L827" rel="#L827">827</span>
<span id="L828" rel="#L828">828</span>
<span id="L829" rel="#L829">829</span>
<span id="L830" rel="#L830">830</span>
<span id="L831" rel="#L831">831</span>
<span id="L832" rel="#L832">832</span>
<span id="L833" rel="#L833">833</span>
<span id="L834" rel="#L834">834</span>
<span id="L835" rel="#L835">835</span>
<span id="L836" rel="#L836">836</span>
<span id="L837" rel="#L837">837</span>
<span id="L838" rel="#L838">838</span>
<span id="L839" rel="#L839">839</span>
<span id="L840" rel="#L840">840</span>
<span id="L841" rel="#L841">841</span>
<span id="L842" rel="#L842">842</span>
<span id="L843" rel="#L843">843</span>
<span id="L844" rel="#L844">844</span>
<span id="L845" rel="#L845">845</span>
<span id="L846" rel="#L846">846</span>
<span id="L847" rel="#L847">847</span>
<span id="L848" rel="#L848">848</span>
<span id="L849" rel="#L849">849</span>
<span id="L850" rel="#L850">850</span>
<span id="L851" rel="#L851">851</span>
<span id="L852" rel="#L852">852</span>
<span id="L853" rel="#L853">853</span>
<span id="L854" rel="#L854">854</span>
<span id="L855" rel="#L855">855</span>
<span id="L856" rel="#L856">856</span>
<span id="L857" rel="#L857">857</span>
<span id="L858" rel="#L858">858</span>
<span id="L859" rel="#L859">859</span>
<span id="L860" rel="#L860">860</span>
<span id="L861" rel="#L861">861</span>
<span id="L862" rel="#L862">862</span>
<span id="L863" rel="#L863">863</span>
<span id="L864" rel="#L864">864</span>
<span id="L865" rel="#L865">865</span>
<span id="L866" rel="#L866">866</span>
<span id="L867" rel="#L867">867</span>
<span id="L868" rel="#L868">868</span>
<span id="L869" rel="#L869">869</span>
<span id="L870" rel="#L870">870</span>
<span id="L871" rel="#L871">871</span>
<span id="L872" rel="#L872">872</span>
<span id="L873" rel="#L873">873</span>
<span id="L874" rel="#L874">874</span>
<span id="L875" rel="#L875">875</span>
<span id="L876" rel="#L876">876</span>
<span id="L877" rel="#L877">877</span>
<span id="L878" rel="#L878">878</span>
<span id="L879" rel="#L879">879</span>
<span id="L880" rel="#L880">880</span>
<span id="L881" rel="#L881">881</span>
<span id="L882" rel="#L882">882</span>
<span id="L883" rel="#L883">883</span>
<span id="L884" rel="#L884">884</span>
<span id="L885" rel="#L885">885</span>
<span id="L886" rel="#L886">886</span>
<span id="L887" rel="#L887">887</span>
<span id="L888" rel="#L888">888</span>
<span id="L889" rel="#L889">889</span>
<span id="L890" rel="#L890">890</span>
<span id="L891" rel="#L891">891</span>
<span id="L892" rel="#L892">892</span>
<span id="L893" rel="#L893">893</span>
<span id="L894" rel="#L894">894</span>
<span id="L895" rel="#L895">895</span>
<span id="L896" rel="#L896">896</span>
<span id="L897" rel="#L897">897</span>
<span id="L898" rel="#L898">898</span>
<span id="L899" rel="#L899">899</span>
<span id="L900" rel="#L900">900</span>
<span id="L901" rel="#L901">901</span>
<span id="L902" rel="#L902">902</span>
<span id="L903" rel="#L903">903</span>
<span id="L904" rel="#L904">904</span>
<span id="L905" rel="#L905">905</span>
<span id="L906" rel="#L906">906</span>
<span id="L907" rel="#L907">907</span>
<span id="L908" rel="#L908">908</span>
<span id="L909" rel="#L909">909</span>
<span id="L910" rel="#L910">910</span>
<span id="L911" rel="#L911">911</span>
<span id="L912" rel="#L912">912</span>
<span id="L913" rel="#L913">913</span>
<span id="L914" rel="#L914">914</span>
<span id="L915" rel="#L915">915</span>
<span id="L916" rel="#L916">916</span>
<span id="L917" rel="#L917">917</span>
<span id="L918" rel="#L918">918</span>
<span id="L919" rel="#L919">919</span>
<span id="L920" rel="#L920">920</span>
<span id="L921" rel="#L921">921</span>
<span id="L922" rel="#L922">922</span>
<span id="L923" rel="#L923">923</span>
<span id="L924" rel="#L924">924</span>
<span id="L925" rel="#L925">925</span>
<span id="L926" rel="#L926">926</span>
<span id="L927" rel="#L927">927</span>
<span id="L928" rel="#L928">928</span>
<span id="L929" rel="#L929">929</span>
<span id="L930" rel="#L930">930</span>
<span id="L931" rel="#L931">931</span>
<span id="L932" rel="#L932">932</span>
<span id="L933" rel="#L933">933</span>
<span id="L934" rel="#L934">934</span>
<span id="L935" rel="#L935">935</span>
<span id="L936" rel="#L936">936</span>
<span id="L937" rel="#L937">937</span>
<span id="L938" rel="#L938">938</span>
<span id="L939" rel="#L939">939</span>
<span id="L940" rel="#L940">940</span>
<span id="L941" rel="#L941">941</span>
<span id="L942" rel="#L942">942</span>
<span id="L943" rel="#L943">943</span>
<span id="L944" rel="#L944">944</span>
<span id="L945" rel="#L945">945</span>
<span id="L946" rel="#L946">946</span>
<span id="L947" rel="#L947">947</span>
<span id="L948" rel="#L948">948</span>
<span id="L949" rel="#L949">949</span>
<span id="L950" rel="#L950">950</span>
<span id="L951" rel="#L951">951</span>
<span id="L952" rel="#L952">952</span>
<span id="L953" rel="#L953">953</span>
<span id="L954" rel="#L954">954</span>
<span id="L955" rel="#L955">955</span>
<span id="L956" rel="#L956">956</span>
<span id="L957" rel="#L957">957</span>
<span id="L958" rel="#L958">958</span>
<span id="L959" rel="#L959">959</span>
<span id="L960" rel="#L960">960</span>
<span id="L961" rel="#L961">961</span>
<span id="L962" rel="#L962">962</span>
<span id="L963" rel="#L963">963</span>
<span id="L964" rel="#L964">964</span>
<span id="L965" rel="#L965">965</span>
<span id="L966" rel="#L966">966</span>
<span id="L967" rel="#L967">967</span>
<span id="L968" rel="#L968">968</span>
<span id="L969" rel="#L969">969</span>
<span id="L970" rel="#L970">970</span>
<span id="L971" rel="#L971">971</span>
<span id="L972" rel="#L972">972</span>
<span id="L973" rel="#L973">973</span>
<span id="L974" rel="#L974">974</span>
<span id="L975" rel="#L975">975</span>
<span id="L976" rel="#L976">976</span>
<span id="L977" rel="#L977">977</span>
<span id="L978" rel="#L978">978</span>
<span id="L979" rel="#L979">979</span>
<span id="L980" rel="#L980">980</span>
<span id="L981" rel="#L981">981</span>
<span id="L982" rel="#L982">982</span>
<span id="L983" rel="#L983">983</span>
<span id="L984" rel="#L984">984</span>
<span id="L985" rel="#L985">985</span>
<span id="L986" rel="#L986">986</span>
<span id="L987" rel="#L987">987</span>
<span id="L988" rel="#L988">988</span>
<span id="L989" rel="#L989">989</span>
<span id="L990" rel="#L990">990</span>
<span id="L991" rel="#L991">991</span>
<span id="L992" rel="#L992">992</span>
<span id="L993" rel="#L993">993</span>
<span id="L994" rel="#L994">994</span>
<span id="L995" rel="#L995">995</span>
<span id="L996" rel="#L996">996</span>
<span id="L997" rel="#L997">997</span>
<span id="L998" rel="#L998">998</span>
<span id="L999" rel="#L999">999</span>
<span id="L1000" rel="#L1000">1000</span>
<span id="L1001" rel="#L1001">1001</span>
<span id="L1002" rel="#L1002">1002</span>
<span id="L1003" rel="#L1003">1003</span>
<span id="L1004" rel="#L1004">1004</span>
<span id="L1005" rel="#L1005">1005</span>
<span id="L1006" rel="#L1006">1006</span>
<span id="L1007" rel="#L1007">1007</span>
<span id="L1008" rel="#L1008">1008</span>
<span id="L1009" rel="#L1009">1009</span>
<span id="L1010" rel="#L1010">1010</span>
<span id="L1011" rel="#L1011">1011</span>
<span id="L1012" rel="#L1012">1012</span>
<span id="L1013" rel="#L1013">1013</span>
<span id="L1014" rel="#L1014">1014</span>
<span id="L1015" rel="#L1015">1015</span>
<span id="L1016" rel="#L1016">1016</span>
<span id="L1017" rel="#L1017">1017</span>
<span id="L1018" rel="#L1018">1018</span>
<span id="L1019" rel="#L1019">1019</span>
<span id="L1020" rel="#L1020">1020</span>
<span id="L1021" rel="#L1021">1021</span>
<span id="L1022" rel="#L1022">1022</span>
<span id="L1023" rel="#L1023">1023</span>
<span id="L1024" rel="#L1024">1024</span>
<span id="L1025" rel="#L1025">1025</span>
<span id="L1026" rel="#L1026">1026</span>
<span id="L1027" rel="#L1027">1027</span>
<span id="L1028" rel="#L1028">1028</span>
<span id="L1029" rel="#L1029">1029</span>
<span id="L1030" rel="#L1030">1030</span>
<span id="L1031" rel="#L1031">1031</span>
<span id="L1032" rel="#L1032">1032</span>
<span id="L1033" rel="#L1033">1033</span>
<span id="L1034" rel="#L1034">1034</span>
<span id="L1035" rel="#L1035">1035</span>
<span id="L1036" rel="#L1036">1036</span>
<span id="L1037" rel="#L1037">1037</span>
<span id="L1038" rel="#L1038">1038</span>
<span id="L1039" rel="#L1039">1039</span>
<span id="L1040" rel="#L1040">1040</span>
<span id="L1041" rel="#L1041">1041</span>
<span id="L1042" rel="#L1042">1042</span>
<span id="L1043" rel="#L1043">1043</span>
<span id="L1044" rel="#L1044">1044</span>
<span id="L1045" rel="#L1045">1045</span>
<span id="L1046" rel="#L1046">1046</span>
<span id="L1047" rel="#L1047">1047</span>
<span id="L1048" rel="#L1048">1048</span>
<span id="L1049" rel="#L1049">1049</span>
<span id="L1050" rel="#L1050">1050</span>
<span id="L1051" rel="#L1051">1051</span>
<span id="L1052" rel="#L1052">1052</span>
<span id="L1053" rel="#L1053">1053</span>
<span id="L1054" rel="#L1054">1054</span>
<span id="L1055" rel="#L1055">1055</span>
<span id="L1056" rel="#L1056">1056</span>
<span id="L1057" rel="#L1057">1057</span>
<span id="L1058" rel="#L1058">1058</span>
<span id="L1059" rel="#L1059">1059</span>
<span id="L1060" rel="#L1060">1060</span>
<span id="L1061" rel="#L1061">1061</span>
<span id="L1062" rel="#L1062">1062</span>
<span id="L1063" rel="#L1063">1063</span>
<span id="L1064" rel="#L1064">1064</span>
<span id="L1065" rel="#L1065">1065</span>
<span id="L1066" rel="#L1066">1066</span>
<span id="L1067" rel="#L1067">1067</span>
<span id="L1068" rel="#L1068">1068</span>
<span id="L1069" rel="#L1069">1069</span>
<span id="L1070" rel="#L1070">1070</span>
<span id="L1071" rel="#L1071">1071</span>
<span id="L1072" rel="#L1072">1072</span>
<span id="L1073" rel="#L1073">1073</span>
<span id="L1074" rel="#L1074">1074</span>
<span id="L1075" rel="#L1075">1075</span>
<span id="L1076" rel="#L1076">1076</span>
<span id="L1077" rel="#L1077">1077</span>
<span id="L1078" rel="#L1078">1078</span>
<span id="L1079" rel="#L1079">1079</span>
<span id="L1080" rel="#L1080">1080</span>
<span id="L1081" rel="#L1081">1081</span>
<span id="L1082" rel="#L1082">1082</span>
<span id="L1083" rel="#L1083">1083</span>
<span id="L1084" rel="#L1084">1084</span>
<span id="L1085" rel="#L1085">1085</span>
<span id="L1086" rel="#L1086">1086</span>
<span id="L1087" rel="#L1087">1087</span>
<span id="L1088" rel="#L1088">1088</span>
<span id="L1089" rel="#L1089">1089</span>
<span id="L1090" rel="#L1090">1090</span>
<span id="L1091" rel="#L1091">1091</span>
<span id="L1092" rel="#L1092">1092</span>
<span id="L1093" rel="#L1093">1093</span>
<span id="L1094" rel="#L1094">1094</span>
<span id="L1095" rel="#L1095">1095</span>
<span id="L1096" rel="#L1096">1096</span>
<span id="L1097" rel="#L1097">1097</span>
<span id="L1098" rel="#L1098">1098</span>
<span id="L1099" rel="#L1099">1099</span>
<span id="L1100" rel="#L1100">1100</span>
<span id="L1101" rel="#L1101">1101</span>
<span id="L1102" rel="#L1102">1102</span>
<span id="L1103" rel="#L1103">1103</span>
<span id="L1104" rel="#L1104">1104</span>
<span id="L1105" rel="#L1105">1105</span>
<span id="L1106" rel="#L1106">1106</span>
<span id="L1107" rel="#L1107">1107</span>
<span id="L1108" rel="#L1108">1108</span>
<span id="L1109" rel="#L1109">1109</span>
<span id="L1110" rel="#L1110">1110</span>
<span id="L1111" rel="#L1111">1111</span>
<span id="L1112" rel="#L1112">1112</span>
<span id="L1113" rel="#L1113">1113</span>
<span id="L1114" rel="#L1114">1114</span>
<span id="L1115" rel="#L1115">1115</span>
<span id="L1116" rel="#L1116">1116</span>
<span id="L1117" rel="#L1117">1117</span>
<span id="L1118" rel="#L1118">1118</span>
<span id="L1119" rel="#L1119">1119</span>
<span id="L1120" rel="#L1120">1120</span>
<span id="L1121" rel="#L1121">1121</span>
<span id="L1122" rel="#L1122">1122</span>
<span id="L1123" rel="#L1123">1123</span>
<span id="L1124" rel="#L1124">1124</span>
<span id="L1125" rel="#L1125">1125</span>
<span id="L1126" rel="#L1126">1126</span>
<span id="L1127" rel="#L1127">1127</span>
<span id="L1128" rel="#L1128">1128</span>
<span id="L1129" rel="#L1129">1129</span>
<span id="L1130" rel="#L1130">1130</span>
<span id="L1131" rel="#L1131">1131</span>
<span id="L1132" rel="#L1132">1132</span>
<span id="L1133" rel="#L1133">1133</span>
<span id="L1134" rel="#L1134">1134</span>
<span id="L1135" rel="#L1135">1135</span>
<span id="L1136" rel="#L1136">1136</span>
<span id="L1137" rel="#L1137">1137</span>
<span id="L1138" rel="#L1138">1138</span>
<span id="L1139" rel="#L1139">1139</span>
<span id="L1140" rel="#L1140">1140</span>
<span id="L1141" rel="#L1141">1141</span>
<span id="L1142" rel="#L1142">1142</span>
<span id="L1143" rel="#L1143">1143</span>
<span id="L1144" rel="#L1144">1144</span>
<span id="L1145" rel="#L1145">1145</span>
<span id="L1146" rel="#L1146">1146</span>
<span id="L1147" rel="#L1147">1147</span>
<span id="L1148" rel="#L1148">1148</span>
<span id="L1149" rel="#L1149">1149</span>
<span id="L1150" rel="#L1150">1150</span>
<span id="L1151" rel="#L1151">1151</span>
<span id="L1152" rel="#L1152">1152</span>
<span id="L1153" rel="#L1153">1153</span>
<span id="L1154" rel="#L1154">1154</span>
<span id="L1155" rel="#L1155">1155</span>
<span id="L1156" rel="#L1156">1156</span>
<span id="L1157" rel="#L1157">1157</span>
<span id="L1158" rel="#L1158">1158</span>
<span id="L1159" rel="#L1159">1159</span>
<span id="L1160" rel="#L1160">1160</span>
<span id="L1161" rel="#L1161">1161</span>
<span id="L1162" rel="#L1162">1162</span>
<span id="L1163" rel="#L1163">1163</span>
<span id="L1164" rel="#L1164">1164</span>
<span id="L1165" rel="#L1165">1165</span>
<span id="L1166" rel="#L1166">1166</span>
<span id="L1167" rel="#L1167">1167</span>
<span id="L1168" rel="#L1168">1168</span>
<span id="L1169" rel="#L1169">1169</span>
<span id="L1170" rel="#L1170">1170</span>
<span id="L1171" rel="#L1171">1171</span>
<span id="L1172" rel="#L1172">1172</span>
<span id="L1173" rel="#L1173">1173</span>
<span id="L1174" rel="#L1174">1174</span>
<span id="L1175" rel="#L1175">1175</span>
<span id="L1176" rel="#L1176">1176</span>
<span id="L1177" rel="#L1177">1177</span>
<span id="L1178" rel="#L1178">1178</span>
<span id="L1179" rel="#L1179">1179</span>
<span id="L1180" rel="#L1180">1180</span>
<span id="L1181" rel="#L1181">1181</span>
<span id="L1182" rel="#L1182">1182</span>
<span id="L1183" rel="#L1183">1183</span>
<span id="L1184" rel="#L1184">1184</span>
<span id="L1185" rel="#L1185">1185</span>
<span id="L1186" rel="#L1186">1186</span>
<span id="L1187" rel="#L1187">1187</span>
<span id="L1188" rel="#L1188">1188</span>
<span id="L1189" rel="#L1189">1189</span>
<span id="L1190" rel="#L1190">1190</span>
<span id="L1191" rel="#L1191">1191</span>
<span id="L1192" rel="#L1192">1192</span>
<span id="L1193" rel="#L1193">1193</span>
<span id="L1194" rel="#L1194">1194</span>
<span id="L1195" rel="#L1195">1195</span>
<span id="L1196" rel="#L1196">1196</span>
<span id="L1197" rel="#L1197">1197</span>
<span id="L1198" rel="#L1198">1198</span>
<span id="L1199" rel="#L1199">1199</span>
<span id="L1200" rel="#L1200">1200</span>
<span id="L1201" rel="#L1201">1201</span>
<span id="L1202" rel="#L1202">1202</span>
<span id="L1203" rel="#L1203">1203</span>
<span id="L1204" rel="#L1204">1204</span>
<span id="L1205" rel="#L1205">1205</span>
<span id="L1206" rel="#L1206">1206</span>
<span id="L1207" rel="#L1207">1207</span>
<span id="L1208" rel="#L1208">1208</span>
<span id="L1209" rel="#L1209">1209</span>
<span id="L1210" rel="#L1210">1210</span>
<span id="L1211" rel="#L1211">1211</span>
<span id="L1212" rel="#L1212">1212</span>
<span id="L1213" rel="#L1213">1213</span>
<span id="L1214" rel="#L1214">1214</span>
<span id="L1215" rel="#L1215">1215</span>
<span id="L1216" rel="#L1216">1216</span>
<span id="L1217" rel="#L1217">1217</span>
<span id="L1218" rel="#L1218">1218</span>
<span id="L1219" rel="#L1219">1219</span>
<span id="L1220" rel="#L1220">1220</span>
<span id="L1221" rel="#L1221">1221</span>
<span id="L1222" rel="#L1222">1222</span>
<span id="L1223" rel="#L1223">1223</span>
<span id="L1224" rel="#L1224">1224</span>
<span id="L1225" rel="#L1225">1225</span>
<span id="L1226" rel="#L1226">1226</span>
<span id="L1227" rel="#L1227">1227</span>
<span id="L1228" rel="#L1228">1228</span>
<span id="L1229" rel="#L1229">1229</span>
<span id="L1230" rel="#L1230">1230</span>
<span id="L1231" rel="#L1231">1231</span>
<span id="L1232" rel="#L1232">1232</span>
<span id="L1233" rel="#L1233">1233</span>
<span id="L1234" rel="#L1234">1234</span>
<span id="L1235" rel="#L1235">1235</span>
<span id="L1236" rel="#L1236">1236</span>
<span id="L1237" rel="#L1237">1237</span>
<span id="L1238" rel="#L1238">1238</span>
<span id="L1239" rel="#L1239">1239</span>
<span id="L1240" rel="#L1240">1240</span>
<span id="L1241" rel="#L1241">1241</span>
<span id="L1242" rel="#L1242">1242</span>
<span id="L1243" rel="#L1243">1243</span>
<span id="L1244" rel="#L1244">1244</span>
<span id="L1245" rel="#L1245">1245</span>
<span id="L1246" rel="#L1246">1246</span>
<span id="L1247" rel="#L1247">1247</span>
<span id="L1248" rel="#L1248">1248</span>
<span id="L1249" rel="#L1249">1249</span>
<span id="L1250" rel="#L1250">1250</span>
<span id="L1251" rel="#L1251">1251</span>
<span id="L1252" rel="#L1252">1252</span>
<span id="L1253" rel="#L1253">1253</span>
<span id="L1254" rel="#L1254">1254</span>
<span id="L1255" rel="#L1255">1255</span>
<span id="L1256" rel="#L1256">1256</span>
<span id="L1257" rel="#L1257">1257</span>
<span id="L1258" rel="#L1258">1258</span>
<span id="L1259" rel="#L1259">1259</span>
<span id="L1260" rel="#L1260">1260</span>
<span id="L1261" rel="#L1261">1261</span>
<span id="L1262" rel="#L1262">1262</span>
<span id="L1263" rel="#L1263">1263</span>
<span id="L1264" rel="#L1264">1264</span>
<span id="L1265" rel="#L1265">1265</span>
<span id="L1266" rel="#L1266">1266</span>
<span id="L1267" rel="#L1267">1267</span>
<span id="L1268" rel="#L1268">1268</span>
<span id="L1269" rel="#L1269">1269</span>
<span id="L1270" rel="#L1270">1270</span>
<span id="L1271" rel="#L1271">1271</span>
<span id="L1272" rel="#L1272">1272</span>
<span id="L1273" rel="#L1273">1273</span>
<span id="L1274" rel="#L1274">1274</span>
<span id="L1275" rel="#L1275">1275</span>
<span id="L1276" rel="#L1276">1276</span>
<span id="L1277" rel="#L1277">1277</span>
<span id="L1278" rel="#L1278">1278</span>
<span id="L1279" rel="#L1279">1279</span>
<span id="L1280" rel="#L1280">1280</span>
<span id="L1281" rel="#L1281">1281</span>
<span id="L1282" rel="#L1282">1282</span>
<span id="L1283" rel="#L1283">1283</span>
<span id="L1284" rel="#L1284">1284</span>
<span id="L1285" rel="#L1285">1285</span>
<span id="L1286" rel="#L1286">1286</span>
<span id="L1287" rel="#L1287">1287</span>
<span id="L1288" rel="#L1288">1288</span>
<span id="L1289" rel="#L1289">1289</span>
<span id="L1290" rel="#L1290">1290</span>
<span id="L1291" rel="#L1291">1291</span>
<span id="L1292" rel="#L1292">1292</span>
<span id="L1293" rel="#L1293">1293</span>
<span id="L1294" rel="#L1294">1294</span>
<span id="L1295" rel="#L1295">1295</span>
<span id="L1296" rel="#L1296">1296</span>
<span id="L1297" rel="#L1297">1297</span>
<span id="L1298" rel="#L1298">1298</span>
<span id="L1299" rel="#L1299">1299</span>
<span id="L1300" rel="#L1300">1300</span>
<span id="L1301" rel="#L1301">1301</span>
<span id="L1302" rel="#L1302">1302</span>
<span id="L1303" rel="#L1303">1303</span>
<span id="L1304" rel="#L1304">1304</span>
<span id="L1305" rel="#L1305">1305</span>
<span id="L1306" rel="#L1306">1306</span>
<span id="L1307" rel="#L1307">1307</span>
<span id="L1308" rel="#L1308">1308</span>
<span id="L1309" rel="#L1309">1309</span>
<span id="L1310" rel="#L1310">1310</span>
<span id="L1311" rel="#L1311">1311</span>
<span id="L1312" rel="#L1312">1312</span>
<span id="L1313" rel="#L1313">1313</span>
<span id="L1314" rel="#L1314">1314</span>
<span id="L1315" rel="#L1315">1315</span>
<span id="L1316" rel="#L1316">1316</span>
<span id="L1317" rel="#L1317">1317</span>
<span id="L1318" rel="#L1318">1318</span>
<span id="L1319" rel="#L1319">1319</span>
<span id="L1320" rel="#L1320">1320</span>
<span id="L1321" rel="#L1321">1321</span>
<span id="L1322" rel="#L1322">1322</span>
<span id="L1323" rel="#L1323">1323</span>
<span id="L1324" rel="#L1324">1324</span>
<span id="L1325" rel="#L1325">1325</span>
<span id="L1326" rel="#L1326">1326</span>
<span id="L1327" rel="#L1327">1327</span>
<span id="L1328" rel="#L1328">1328</span>
<span id="L1329" rel="#L1329">1329</span>
<span id="L1330" rel="#L1330">1330</span>
<span id="L1331" rel="#L1331">1331</span>
<span id="L1332" rel="#L1332">1332</span>
<span id="L1333" rel="#L1333">1333</span>
<span id="L1334" rel="#L1334">1334</span>
<span id="L1335" rel="#L1335">1335</span>
<span id="L1336" rel="#L1336">1336</span>
<span id="L1337" rel="#L1337">1337</span>
<span id="L1338" rel="#L1338">1338</span>
<span id="L1339" rel="#L1339">1339</span>
<span id="L1340" rel="#L1340">1340</span>
<span id="L1341" rel="#L1341">1341</span>
<span id="L1342" rel="#L1342">1342</span>
<span id="L1343" rel="#L1343">1343</span>
<span id="L1344" rel="#L1344">1344</span>
<span id="L1345" rel="#L1345">1345</span>
<span id="L1346" rel="#L1346">1346</span>
<span id="L1347" rel="#L1347">1347</span>
<span id="L1348" rel="#L1348">1348</span>
<span id="L1349" rel="#L1349">1349</span>
<span id="L1350" rel="#L1350">1350</span>
<span id="L1351" rel="#L1351">1351</span>
<span id="L1352" rel="#L1352">1352</span>
<span id="L1353" rel="#L1353">1353</span>
<span id="L1354" rel="#L1354">1354</span>
<span id="L1355" rel="#L1355">1355</span>
<span id="L1356" rel="#L1356">1356</span>
<span id="L1357" rel="#L1357">1357</span>
<span id="L1358" rel="#L1358">1358</span>
<span id="L1359" rel="#L1359">1359</span>
<span id="L1360" rel="#L1360">1360</span>
<span id="L1361" rel="#L1361">1361</span>
<span id="L1362" rel="#L1362">1362</span>
<span id="L1363" rel="#L1363">1363</span>
<span id="L1364" rel="#L1364">1364</span>
<span id="L1365" rel="#L1365">1365</span>
<span id="L1366" rel="#L1366">1366</span>
<span id="L1367" rel="#L1367">1367</span>
<span id="L1368" rel="#L1368">1368</span>
<span id="L1369" rel="#L1369">1369</span>
<span id="L1370" rel="#L1370">1370</span>
<span id="L1371" rel="#L1371">1371</span>
<span id="L1372" rel="#L1372">1372</span>
<span id="L1373" rel="#L1373">1373</span>
<span id="L1374" rel="#L1374">1374</span>
<span id="L1375" rel="#L1375">1375</span>
<span id="L1376" rel="#L1376">1376</span>
<span id="L1377" rel="#L1377">1377</span>
<span id="L1378" rel="#L1378">1378</span>
<span id="L1379" rel="#L1379">1379</span>
<span id="L1380" rel="#L1380">1380</span>
<span id="L1381" rel="#L1381">1381</span>
<span id="L1382" rel="#L1382">1382</span>
<span id="L1383" rel="#L1383">1383</span>
<span id="L1384" rel="#L1384">1384</span>
<span id="L1385" rel="#L1385">1385</span>
<span id="L1386" rel="#L1386">1386</span>
<span id="L1387" rel="#L1387">1387</span>
<span id="L1388" rel="#L1388">1388</span>
<span id="L1389" rel="#L1389">1389</span>
<span id="L1390" rel="#L1390">1390</span>
<span id="L1391" rel="#L1391">1391</span>
<span id="L1392" rel="#L1392">1392</span>
<span id="L1393" rel="#L1393">1393</span>
<span id="L1394" rel="#L1394">1394</span>
<span id="L1395" rel="#L1395">1395</span>
<span id="L1396" rel="#L1396">1396</span>
<span id="L1397" rel="#L1397">1397</span>
<span id="L1398" rel="#L1398">1398</span>
<span id="L1399" rel="#L1399">1399</span>
<span id="L1400" rel="#L1400">1400</span>
<span id="L1401" rel="#L1401">1401</span>
<span id="L1402" rel="#L1402">1402</span>
<span id="L1403" rel="#L1403">1403</span>
<span id="L1404" rel="#L1404">1404</span>
<span id="L1405" rel="#L1405">1405</span>
<span id="L1406" rel="#L1406">1406</span>
<span id="L1407" rel="#L1407">1407</span>
<span id="L1408" rel="#L1408">1408</span>
<span id="L1409" rel="#L1409">1409</span>
<span id="L1410" rel="#L1410">1410</span>
<span id="L1411" rel="#L1411">1411</span>
<span id="L1412" rel="#L1412">1412</span>
<span id="L1413" rel="#L1413">1413</span>
<span id="L1414" rel="#L1414">1414</span>
<span id="L1415" rel="#L1415">1415</span>
<span id="L1416" rel="#L1416">1416</span>
<span id="L1417" rel="#L1417">1417</span>
<span id="L1418" rel="#L1418">1418</span>
<span id="L1419" rel="#L1419">1419</span>
<span id="L1420" rel="#L1420">1420</span>
<span id="L1421" rel="#L1421">1421</span>
<span id="L1422" rel="#L1422">1422</span>
<span id="L1423" rel="#L1423">1423</span>
<span id="L1424" rel="#L1424">1424</span>
<span id="L1425" rel="#L1425">1425</span>
<span id="L1426" rel="#L1426">1426</span>
<span id="L1427" rel="#L1427">1427</span>
<span id="L1428" rel="#L1428">1428</span>
<span id="L1429" rel="#L1429">1429</span>
<span id="L1430" rel="#L1430">1430</span>
<span id="L1431" rel="#L1431">1431</span>
<span id="L1432" rel="#L1432">1432</span>
<span id="L1433" rel="#L1433">1433</span>
<span id="L1434" rel="#L1434">1434</span>
<span id="L1435" rel="#L1435">1435</span>
<span id="L1436" rel="#L1436">1436</span>
<span id="L1437" rel="#L1437">1437</span>
<span id="L1438" rel="#L1438">1438</span>
<span id="L1439" rel="#L1439">1439</span>
<span id="L1440" rel="#L1440">1440</span>
<span id="L1441" rel="#L1441">1441</span>
<span id="L1442" rel="#L1442">1442</span>
<span id="L1443" rel="#L1443">1443</span>
<span id="L1444" rel="#L1444">1444</span>
<span id="L1445" rel="#L1445">1445</span>
<span id="L1446" rel="#L1446">1446</span>
<span id="L1447" rel="#L1447">1447</span>
<span id="L1448" rel="#L1448">1448</span>
<span id="L1449" rel="#L1449">1449</span>
<span id="L1450" rel="#L1450">1450</span>
<span id="L1451" rel="#L1451">1451</span>
<span id="L1452" rel="#L1452">1452</span>
<span id="L1453" rel="#L1453">1453</span>
<span id="L1454" rel="#L1454">1454</span>
<span id="L1455" rel="#L1455">1455</span>
<span id="L1456" rel="#L1456">1456</span>
<span id="L1457" rel="#L1457">1457</span>
<span id="L1458" rel="#L1458">1458</span>
<span id="L1459" rel="#L1459">1459</span>
<span id="L1460" rel="#L1460">1460</span>
<span id="L1461" rel="#L1461">1461</span>
<span id="L1462" rel="#L1462">1462</span>
<span id="L1463" rel="#L1463">1463</span>
<span id="L1464" rel="#L1464">1464</span>
<span id="L1465" rel="#L1465">1465</span>
<span id="L1466" rel="#L1466">1466</span>
<span id="L1467" rel="#L1467">1467</span>
<span id="L1468" rel="#L1468">1468</span>
<span id="L1469" rel="#L1469">1469</span>
<span id="L1470" rel="#L1470">1470</span>
<span id="L1471" rel="#L1471">1471</span>
<span id="L1472" rel="#L1472">1472</span>
<span id="L1473" rel="#L1473">1473</span>
<span id="L1474" rel="#L1474">1474</span>
<span id="L1475" rel="#L1475">1475</span>
<span id="L1476" rel="#L1476">1476</span>
<span id="L1477" rel="#L1477">1477</span>
<span id="L1478" rel="#L1478">1478</span>
<span id="L1479" rel="#L1479">1479</span>
<span id="L1480" rel="#L1480">1480</span>
<span id="L1481" rel="#L1481">1481</span>
<span id="L1482" rel="#L1482">1482</span>
<span id="L1483" rel="#L1483">1483</span>
<span id="L1484" rel="#L1484">1484</span>
<span id="L1485" rel="#L1485">1485</span>
<span id="L1486" rel="#L1486">1486</span>
<span id="L1487" rel="#L1487">1487</span>
<span id="L1488" rel="#L1488">1488</span>
<span id="L1489" rel="#L1489">1489</span>
<span id="L1490" rel="#L1490">1490</span>
<span id="L1491" rel="#L1491">1491</span>
<span id="L1492" rel="#L1492">1492</span>
<span id="L1493" rel="#L1493">1493</span>
<span id="L1494" rel="#L1494">1494</span>
<span id="L1495" rel="#L1495">1495</span>
<span id="L1496" rel="#L1496">1496</span>
<span id="L1497" rel="#L1497">1497</span>
<span id="L1498" rel="#L1498">1498</span>
<span id="L1499" rel="#L1499">1499</span>
<span id="L1500" rel="#L1500">1500</span>
<span id="L1501" rel="#L1501">1501</span>
<span id="L1502" rel="#L1502">1502</span>
<span id="L1503" rel="#L1503">1503</span>
<span id="L1504" rel="#L1504">1504</span>
<span id="L1505" rel="#L1505">1505</span>
<span id="L1506" rel="#L1506">1506</span>
<span id="L1507" rel="#L1507">1507</span>
<span id="L1508" rel="#L1508">1508</span>
<span id="L1509" rel="#L1509">1509</span>
<span id="L1510" rel="#L1510">1510</span>
<span id="L1511" rel="#L1511">1511</span>
<span id="L1512" rel="#L1512">1512</span>
<span id="L1513" rel="#L1513">1513</span>
<span id="L1514" rel="#L1514">1514</span>
<span id="L1515" rel="#L1515">1515</span>
<span id="L1516" rel="#L1516">1516</span>
<span id="L1517" rel="#L1517">1517</span>
<span id="L1518" rel="#L1518">1518</span>
<span id="L1519" rel="#L1519">1519</span>
<span id="L1520" rel="#L1520">1520</span>
<span id="L1521" rel="#L1521">1521</span>
<span id="L1522" rel="#L1522">1522</span>
<span id="L1523" rel="#L1523">1523</span>
<span id="L1524" rel="#L1524">1524</span>
<span id="L1525" rel="#L1525">1525</span>
<span id="L1526" rel="#L1526">1526</span>
<span id="L1527" rel="#L1527">1527</span>
<span id="L1528" rel="#L1528">1528</span>
<span id="L1529" rel="#L1529">1529</span>
<span id="L1530" rel="#L1530">1530</span>
<span id="L1531" rel="#L1531">1531</span>
<span id="L1532" rel="#L1532">1532</span>
<span id="L1533" rel="#L1533">1533</span>
<span id="L1534" rel="#L1534">1534</span>
<span id="L1535" rel="#L1535">1535</span>
<span id="L1536" rel="#L1536">1536</span>
<span id="L1537" rel="#L1537">1537</span>
<span id="L1538" rel="#L1538">1538</span>
<span id="L1539" rel="#L1539">1539</span>
<span id="L1540" rel="#L1540">1540</span>
<span id="L1541" rel="#L1541">1541</span>
<span id="L1542" rel="#L1542">1542</span>
<span id="L1543" rel="#L1543">1543</span>
<span id="L1544" rel="#L1544">1544</span>
<span id="L1545" rel="#L1545">1545</span>
<span id="L1546" rel="#L1546">1546</span>
<span id="L1547" rel="#L1547">1547</span>
<span id="L1548" rel="#L1548">1548</span>
<span id="L1549" rel="#L1549">1549</span>
<span id="L1550" rel="#L1550">1550</span>
<span id="L1551" rel="#L1551">1551</span>
<span id="L1552" rel="#L1552">1552</span>
<span id="L1553" rel="#L1553">1553</span>
<span id="L1554" rel="#L1554">1554</span>
<span id="L1555" rel="#L1555">1555</span>
<span id="L1556" rel="#L1556">1556</span>
<span id="L1557" rel="#L1557">1557</span>
<span id="L1558" rel="#L1558">1558</span>
<span id="L1559" rel="#L1559">1559</span>
<span id="L1560" rel="#L1560">1560</span>
<span id="L1561" rel="#L1561">1561</span>
<span id="L1562" rel="#L1562">1562</span>
<span id="L1563" rel="#L1563">1563</span>
</pre>
          </td>
          <td width="100%">
            
              
                <div class="highlight"><pre><div class='line' id='LC1'><span class="c1">// Copyright (c) 2009-2010 Satoshi Nakamoto</span></div><div class='line' id='LC2'><span class="c1">// Distributed under the MIT/X11 software license, see the accompanying</span></div><div class='line' id='LC3'><span class="c1">// file license.txt or http://www.opensource.org/licenses/mit-license.php.</span></div><div class='line' id='LC4'><br/></div><div class='line' id='LC5'><span class="cp">#include &quot;headers.h&quot;</span></div><div class='line' id='LC6'><br/></div><div class='line' id='LC7'><span class="cp">#ifdef USE_UPNP</span></div><div class='line' id='LC8'><span class="cp">#include &lt;miniupnpc/miniwget.h&gt;</span></div><div class='line' id='LC9'><span class="cp">#include &lt;miniupnpc/miniupnpc.h&gt;</span></div><div class='line' id='LC10'><span class="cp">#include &lt;miniupnpc/upnpcommands.h&gt;</span></div><div class='line' id='LC11'><span class="cp">#include &lt;miniupnpc/upnperrors.h&gt;</span></div><div class='line' id='LC12'><span class="cp">#endif</span></div><div class='line' id='LC13'><br/></div><div class='line' id='LC14'><span class="k">static</span> <span class="k">const</span> <span class="kt">int</span> <span class="n">MAX_OUTBOUND_CONNECTIONS</span> <span class="o">=</span> <span class="mi">8</span><span class="p">;</span></div><div class='line' id='LC15'><br/></div><div class='line' id='LC16'><span class="kt">void</span> <span class="n">ThreadMessageHandler2</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">);</span></div><div class='line' id='LC17'><span class="kt">void</span> <span class="n">ThreadSocketHandler2</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">);</span></div><div class='line' id='LC18'><span class="kt">void</span> <span class="n">ThreadOpenConnections2</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">);</span></div><div class='line' id='LC19'><span class="cp">#ifdef USE_UPNP</span></div><div class='line' id='LC20'><span class="kt">void</span> <span class="n">ThreadMapPort2</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">);</span></div><div class='line' id='LC21'><span class="cp">#endif</span></div><div class='line' id='LC22'><span class="kt">bool</span> <span class="n">OpenNetworkConnection</span><span class="p">(</span><span class="k">const</span> <span class="n">CAddress</span><span class="o">&amp;</span> <span class="n">addrConnect</span><span class="p">);</span></div><div class='line' id='LC23'><br/></div><div class='line' id='LC24'><br/></div><div class='line' id='LC25'><br/></div><div class='line' id='LC26'><br/></div><div class='line' id='LC27'><br/></div><div class='line' id='LC28'><span class="c1">//</span></div><div class='line' id='LC29'><span class="c1">// Global state variables</span></div><div class='line' id='LC30'><span class="c1">//</span></div><div class='line' id='LC31'><span class="kt">bool</span> <span class="n">fClient</span> <span class="o">=</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC32'><span class="n">uint64</span> <span class="n">nLocalServices</span> <span class="o">=</span> <span class="p">(</span><span class="n">fClient</span> <span class="o">?</span> <span class="mi">0</span> <span class="o">:</span> <span class="n">NODE_NETWORK</span><span class="p">);</span></div><div class='line' id='LC33'><span class="n">CAddress</span> <span class="n">addrLocalHost</span><span class="p">(</span><span class="mi">0</span><span class="p">,</span> <span class="mi">0</span><span class="p">,</span> <span class="n">nLocalServices</span><span class="p">);</span></div><div class='line' id='LC34'><span class="n">CNode</span><span class="o">*</span> <span class="n">pnodeLocalHost</span> <span class="o">=</span> <span class="nb">NULL</span><span class="p">;</span></div><div class='line' id='LC35'><span class="n">uint64</span> <span class="n">nLocalHostNonce</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC36'><span class="n">array</span><span class="o">&lt;</span><span class="kt">int</span><span class="p">,</span> <span class="mi">10</span><span class="o">&gt;</span> <span class="n">vnThreadsRunning</span><span class="p">;</span></div><div class='line' id='LC37'><span class="n">SOCKET</span> <span class="n">hListenSocket</span> <span class="o">=</span> <span class="n">INVALID_SOCKET</span><span class="p">;</span></div><div class='line' id='LC38'><br/></div><div class='line' id='LC39'><span class="n">vector</span><span class="o">&lt;</span><span class="n">CNode</span><span class="o">*&gt;</span> <span class="n">vNodes</span><span class="p">;</span></div><div class='line' id='LC40'><span class="n">CCriticalSection</span> <span class="n">cs_vNodes</span><span class="p">;</span></div><div class='line' id='LC41'><span class="n">map</span><span class="o">&lt;</span><span class="n">vector</span><span class="o">&lt;</span><span class="kt">unsigned</span> <span class="kt">char</span><span class="o">&gt;</span><span class="p">,</span> <span class="n">CAddress</span><span class="o">&gt;</span> <span class="n">mapAddresses</span><span class="p">;</span></div><div class='line' id='LC42'><span class="n">CCriticalSection</span> <span class="n">cs_mapAddresses</span><span class="p">;</span></div><div class='line' id='LC43'><span class="n">map</span><span class="o">&lt;</span><span class="n">CInv</span><span class="p">,</span> <span class="n">CDataStream</span><span class="o">&gt;</span> <span class="n">mapRelay</span><span class="p">;</span></div><div class='line' id='LC44'><span class="n">deque</span><span class="o">&lt;</span><span class="n">pair</span><span class="o">&lt;</span><span class="n">int64</span><span class="p">,</span> <span class="n">CInv</span><span class="o">&gt;</span> <span class="o">&gt;</span> <span class="n">vRelayExpiration</span><span class="p">;</span></div><div class='line' id='LC45'><span class="n">CCriticalSection</span> <span class="n">cs_mapRelay</span><span class="p">;</span></div><div class='line' id='LC46'><span class="n">map</span><span class="o">&lt;</span><span class="n">CInv</span><span class="p">,</span> <span class="n">int64</span><span class="o">&gt;</span> <span class="n">mapAlreadyAskedFor</span><span class="p">;</span></div><div class='line' id='LC47'><br/></div><div class='line' id='LC48'><span class="c1">// Settings</span></div><div class='line' id='LC49'><span class="kt">int</span> <span class="n">fUseProxy</span> <span class="o">=</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC50'><span class="n">CAddress</span> <span class="n">addrProxy</span><span class="p">(</span><span class="s">&quot;127.0.0.1:9050&quot;</span><span class="p">);</span></div><div class='line' id='LC51'><br/></div><div class='line' id='LC52'><br/></div><div class='line' id='LC53'><br/></div><div class='line' id='LC54'><br/></div><div class='line' id='LC55'><br/></div><div class='line' id='LC56'><span class="kt">void</span> <span class="n">CNode</span><span class="o">::</span><span class="n">PushGetBlocks</span><span class="p">(</span><span class="n">CBlockIndex</span><span class="o">*</span> <span class="n">pindexBegin</span><span class="p">,</span> <span class="n">uint256</span> <span class="n">hashEnd</span><span class="p">)</span></div><div class='line' id='LC57'><span class="p">{</span></div><div class='line' id='LC58'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Filter out duplicate requests</span></div><div class='line' id='LC59'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pindexBegin</span> <span class="o">==</span> <span class="n">pindexLastGetBlocksBegin</span> <span class="o">&amp;&amp;</span> <span class="n">hashEnd</span> <span class="o">==</span> <span class="n">hashLastGetBlocksEnd</span><span class="p">)</span></div><div class='line' id='LC60'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC61'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pindexLastGetBlocksBegin</span> <span class="o">=</span> <span class="n">pindexBegin</span><span class="p">;</span></div><div class='line' id='LC62'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">hashLastGetBlocksEnd</span> <span class="o">=</span> <span class="n">hashEnd</span><span class="p">;</span></div><div class='line' id='LC63'><br/></div><div class='line' id='LC64'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PushMessage</span><span class="p">(</span><span class="s">&quot;getblocks&quot;</span><span class="p">,</span> <span class="n">CBlockLocator</span><span class="p">(</span><span class="n">pindexBegin</span><span class="p">),</span> <span class="n">hashEnd</span><span class="p">);</span></div><div class='line' id='LC65'><span class="p">}</span></div><div class='line' id='LC66'><br/></div><div class='line' id='LC67'><br/></div><div class='line' id='LC68'><br/></div><div class='line' id='LC69'><br/></div><div class='line' id='LC70'><br/></div><div class='line' id='LC71'><span class="kt">bool</span> <span class="n">ConnectSocket</span><span class="p">(</span><span class="k">const</span> <span class="n">CAddress</span><span class="o">&amp;</span> <span class="n">addrConnect</span><span class="p">,</span> <span class="n">SOCKET</span><span class="o">&amp;</span> <span class="n">hSocketRet</span><span class="p">)</span></div><div class='line' id='LC72'><span class="p">{</span></div><div class='line' id='LC73'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">hSocketRet</span> <span class="o">=</span> <span class="n">INVALID_SOCKET</span><span class="p">;</span></div><div class='line' id='LC74'><br/></div><div class='line' id='LC75'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">SOCKET</span> <span class="n">hSocket</span> <span class="o">=</span> <span class="n">socket</span><span class="p">(</span><span class="n">AF_INET</span><span class="p">,</span> <span class="n">SOCK_STREAM</span><span class="p">,</span> <span class="n">IPPROTO_TCP</span><span class="p">);</span></div><div class='line' id='LC76'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">hSocket</span> <span class="o">==</span> <span class="n">INVALID_SOCKET</span><span class="p">)</span></div><div class='line' id='LC77'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC78'><span class="cp">#ifdef BSD</span></div><div class='line' id='LC79'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">set</span> <span class="o">=</span> <span class="mi">1</span><span class="p">;</span></div><div class='line' id='LC80'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">setsockopt</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">SOL_SOCKET</span><span class="p">,</span> <span class="n">SO_NOSIGPIPE</span><span class="p">,</span> <span class="p">(</span><span class="kt">void</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="n">set</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="kt">int</span><span class="p">));</span></div><div class='line' id='LC81'><span class="cp">#endif</span></div><div class='line' id='LC82'><br/></div><div class='line' id='LC83'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">bool</span> <span class="n">fRoutable</span> <span class="o">=</span> <span class="o">!</span><span class="p">(</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">GetByte</span><span class="p">(</span><span class="mi">3</span><span class="p">)</span> <span class="o">==</span> <span class="mi">10</span> <span class="o">||</span> <span class="p">(</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">GetByte</span><span class="p">(</span><span class="mi">3</span><span class="p">)</span> <span class="o">==</span> <span class="mi">192</span> <span class="o">&amp;&amp;</span> <span class="n">addrConnect</span><span class="p">.</span><span class="n">GetByte</span><span class="p">(</span><span class="mi">2</span><span class="p">)</span> <span class="o">==</span> <span class="mi">168</span><span class="p">));</span></div><div class='line' id='LC84'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">bool</span> <span class="n">fProxy</span> <span class="o">=</span> <span class="p">(</span><span class="n">fUseProxy</span> <span class="o">&amp;&amp;</span> <span class="n">fRoutable</span><span class="p">);</span></div><div class='line' id='LC85'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">sockaddr_in</span> <span class="n">sockaddr</span> <span class="o">=</span> <span class="p">(</span><span class="n">fProxy</span> <span class="o">?</span> <span class="n">addrProxy</span><span class="p">.</span><span class="n">GetSockAddr</span><span class="p">()</span> <span class="o">:</span> <span class="n">addrConnect</span><span class="p">.</span><span class="n">GetSockAddr</span><span class="p">());</span></div><div class='line' id='LC86'><br/></div><div class='line' id='LC87'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">connect</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="p">(</span><span class="k">struct</span> <span class="n">sockaddr</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="n">sockaddr</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">sockaddr</span><span class="p">))</span> <span class="o">==</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC88'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC89'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC90'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC91'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC92'><br/></div><div class='line' id='LC93'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fProxy</span><span class="p">)</span></div><div class='line' id='LC94'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC95'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;proxy connecting %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">addrConnect</span><span class="p">.</span><span class="n">ToStringLog</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC96'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span> <span class="n">pszSocks4IP</span><span class="p">[]</span> <span class="o">=</span> <span class="s">&quot;</span><span class="se">\4\1\0\0\0\0\0\0</span><span class="s">user&quot;</span><span class="p">;</span></div><div class='line' id='LC97'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">memcpy</span><span class="p">(</span><span class="n">pszSocks4IP</span> <span class="o">+</span> <span class="mi">2</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">port</span><span class="p">,</span> <span class="mi">2</span><span class="p">);</span></div><div class='line' id='LC98'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">memcpy</span><span class="p">(</span><span class="n">pszSocks4IP</span> <span class="o">+</span> <span class="mi">4</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">ip</span><span class="p">,</span> <span class="mi">4</span><span class="p">);</span></div><div class='line' id='LC99'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span><span class="o">*</span> <span class="n">pszSocks4</span> <span class="o">=</span> <span class="n">pszSocks4IP</span><span class="p">;</span></div><div class='line' id='LC100'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nSize</span> <span class="o">=</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">pszSocks4IP</span><span class="p">);</span></div><div class='line' id='LC101'><br/></div><div class='line' id='LC102'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">ret</span> <span class="o">=</span> <span class="n">send</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">pszSocks4</span><span class="p">,</span> <span class="n">nSize</span><span class="p">,</span> <span class="n">MSG_NOSIGNAL</span><span class="p">);</span></div><div class='line' id='LC103'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">ret</span> <span class="o">!=</span> <span class="n">nSize</span><span class="p">)</span></div><div class='line' id='LC104'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC105'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC106'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="n">error</span><span class="p">(</span><span class="s">&quot;Error sending to proxy&quot;</span><span class="p">);</span></div><div class='line' id='LC107'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC108'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span> <span class="n">pchRet</span><span class="p">[</span><span class="mi">8</span><span class="p">];</span></div><div class='line' id='LC109'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">recv</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">pchRet</span><span class="p">,</span> <span class="mi">8</span><span class="p">,</span> <span class="mi">0</span><span class="p">)</span> <span class="o">!=</span> <span class="mi">8</span><span class="p">)</span></div><div class='line' id='LC110'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC111'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC112'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="n">error</span><span class="p">(</span><span class="s">&quot;Error reading proxy response&quot;</span><span class="p">);</span></div><div class='line' id='LC113'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC114'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pchRet</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span> <span class="o">!=</span> <span class="mh">0x5a</span><span class="p">)</span></div><div class='line' id='LC115'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC116'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC117'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pchRet</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span> <span class="o">!=</span> <span class="mh">0x5b</span><span class="p">)</span></div><div class='line' id='LC118'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ERROR: Proxy returned error %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">pchRet</span><span class="p">[</span><span class="mi">1</span><span class="p">]);</span></div><div class='line' id='LC119'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC120'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC121'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;proxy connected %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">addrConnect</span><span class="p">.</span><span class="n">ToStringLog</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC122'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC123'><br/></div><div class='line' id='LC124'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">hSocketRet</span> <span class="o">=</span> <span class="n">hSocket</span><span class="p">;</span></div><div class='line' id='LC125'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC126'><span class="p">}</span></div><div class='line' id='LC127'><br/></div><div class='line' id='LC128'><br/></div><div class='line' id='LC129'><br/></div><div class='line' id='LC130'><span class="kt">bool</span> <span class="n">GetMyExternalIP2</span><span class="p">(</span><span class="k">const</span> <span class="n">CAddress</span><span class="o">&amp;</span> <span class="n">addrConnect</span><span class="p">,</span> <span class="k">const</span> <span class="kt">char</span><span class="o">*</span> <span class="n">pszGet</span><span class="p">,</span> <span class="k">const</span> <span class="kt">char</span><span class="o">*</span> <span class="n">pszKeyword</span><span class="p">,</span> <span class="kt">unsigned</span> <span class="kt">int</span><span class="o">&amp;</span> <span class="n">ipRet</span><span class="p">)</span></div><div class='line' id='LC131'><span class="p">{</span></div><div class='line' id='LC132'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">SOCKET</span> <span class="n">hSocket</span><span class="p">;</span></div><div class='line' id='LC133'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">ConnectSocket</span><span class="p">(</span><span class="n">addrConnect</span><span class="p">,</span> <span class="n">hSocket</span><span class="p">))</span></div><div class='line' id='LC134'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="n">error</span><span class="p">(</span><span class="s">&quot;GetMyExternalIP() : connection to %s failed&quot;</span><span class="p">,</span> <span class="n">addrConnect</span><span class="p">.</span><span class="n">ToString</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC135'><br/></div><div class='line' id='LC136'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">send</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">pszGet</span><span class="p">,</span> <span class="n">strlen</span><span class="p">(</span><span class="n">pszGet</span><span class="p">),</span> <span class="n">MSG_NOSIGNAL</span><span class="p">);</span></div><div class='line' id='LC137'><br/></div><div class='line' id='LC138'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">string</span> <span class="n">strLine</span><span class="p">;</span></div><div class='line' id='LC139'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">while</span> <span class="p">(</span><span class="n">RecvLine</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">strLine</span><span class="p">))</span></div><div class='line' id='LC140'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC141'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">strLine</span><span class="p">.</span><span class="n">empty</span><span class="p">())</span> <span class="c1">// HTTP response is separated from headers by blank line</span></div><div class='line' id='LC142'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC143'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">loop</span></div><div class='line' id='LC144'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC145'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">RecvLine</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">strLine</span><span class="p">))</span></div><div class='line' id='LC146'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC147'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC148'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC149'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC150'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pszKeyword</span> <span class="o">==</span> <span class="nb">NULL</span><span class="p">)</span></div><div class='line' id='LC151'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">break</span><span class="p">;</span></div><div class='line' id='LC152'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">strLine</span><span class="p">.</span><span class="n">find</span><span class="p">(</span><span class="n">pszKeyword</span><span class="p">)</span> <span class="o">!=</span> <span class="o">-</span><span class="mi">1</span><span class="p">)</span></div><div class='line' id='LC153'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC154'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strLine</span> <span class="o">=</span> <span class="n">strLine</span><span class="p">.</span><span class="n">substr</span><span class="p">(</span><span class="n">strLine</span><span class="p">.</span><span class="n">find</span><span class="p">(</span><span class="n">pszKeyword</span><span class="p">)</span> <span class="o">+</span> <span class="n">strlen</span><span class="p">(</span><span class="n">pszKeyword</span><span class="p">));</span></div><div class='line' id='LC155'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">break</span><span class="p">;</span></div><div class='line' id='LC156'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC157'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC158'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC159'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">strLine</span><span class="p">.</span><span class="n">find</span><span class="p">(</span><span class="s">&quot;&lt;&quot;</span><span class="p">)</span> <span class="o">!=</span> <span class="o">-</span><span class="mi">1</span><span class="p">)</span></div><div class='line' id='LC160'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strLine</span> <span class="o">=</span> <span class="n">strLine</span><span class="p">.</span><span class="n">substr</span><span class="p">(</span><span class="mi">0</span><span class="p">,</span> <span class="n">strLine</span><span class="p">.</span><span class="n">find</span><span class="p">(</span><span class="s">&quot;&lt;&quot;</span><span class="p">));</span></div><div class='line' id='LC161'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strLine</span> <span class="o">=</span> <span class="n">strLine</span><span class="p">.</span><span class="n">substr</span><span class="p">(</span><span class="n">strspn</span><span class="p">(</span><span class="n">strLine</span><span class="p">.</span><span class="n">c_str</span><span class="p">(),</span> <span class="s">&quot; </span><span class="se">\t\n\r</span><span class="s">&quot;</span><span class="p">));</span></div><div class='line' id='LC162'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">while</span> <span class="p">(</span><span class="n">strLine</span><span class="p">.</span><span class="n">size</span><span class="p">()</span> <span class="o">&gt;</span> <span class="mi">0</span> <span class="o">&amp;&amp;</span> <span class="n">isspace</span><span class="p">(</span><span class="n">strLine</span><span class="p">[</span><span class="n">strLine</span><span class="p">.</span><span class="n">size</span><span class="p">()</span><span class="o">-</span><span class="mi">1</span><span class="p">]))</span></div><div class='line' id='LC163'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strLine</span><span class="p">.</span><span class="n">resize</span><span class="p">(</span><span class="n">strLine</span><span class="p">.</span><span class="n">size</span><span class="p">()</span><span class="o">-</span><span class="mi">1</span><span class="p">);</span></div><div class='line' id='LC164'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addr</span><span class="p">(</span><span class="n">strLine</span><span class="p">.</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC165'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;GetMyExternalIP() received [%s] %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">strLine</span><span class="p">.</span><span class="n">c_str</span><span class="p">(),</span> <span class="n">addr</span><span class="p">.</span><span class="n">ToString</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC166'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">ip</span> <span class="o">==</span> <span class="mi">0</span> <span class="o">||</span> <span class="n">addr</span><span class="p">.</span><span class="n">ip</span> <span class="o">==</span> <span class="n">INADDR_NONE</span> <span class="o">||</span> <span class="o">!</span><span class="n">addr</span><span class="p">.</span><span class="n">IsRoutable</span><span class="p">())</span></div><div class='line' id='LC167'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC168'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ipRet</span> <span class="o">=</span> <span class="n">addr</span><span class="p">.</span><span class="n">ip</span><span class="p">;</span></div><div class='line' id='LC169'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC170'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC171'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC172'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC173'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="n">error</span><span class="p">(</span><span class="s">&quot;GetMyExternalIP() : connection closed&quot;</span><span class="p">);</span></div><div class='line' id='LC174'><span class="p">}</span></div><div class='line' id='LC175'><br/></div><div class='line' id='LC176'><span class="c1">// We now get our external IP from the IRC server first and only use this as a backup</span></div><div class='line' id='LC177'><span class="kt">bool</span> <span class="n">GetMyExternalIP</span><span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span><span class="o">&amp;</span> <span class="n">ipRet</span><span class="p">)</span></div><div class='line' id='LC178'><span class="p">{</span></div><div class='line' id='LC179'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addrConnect</span><span class="p">;</span></div><div class='line' id='LC180'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">const</span> <span class="kt">char</span><span class="o">*</span> <span class="n">pszGet</span><span class="p">;</span></div><div class='line' id='LC181'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">const</span> <span class="kt">char</span><span class="o">*</span> <span class="n">pszKeyword</span><span class="p">;</span></div><div class='line' id='LC182'><br/></div><div class='line' id='LC183'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fUseProxy</span><span class="p">)</span></div><div class='line' id='LC184'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC185'><br/></div><div class='line' id='LC186'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">nLookup</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="n">nLookup</span> <span class="o">&lt;=</span> <span class="mi">1</span><span class="p">;</span> <span class="n">nLookup</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC187'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">nHost</span> <span class="o">=</span> <span class="mi">1</span><span class="p">;</span> <span class="n">nHost</span> <span class="o">&lt;=</span> <span class="mi">2</span><span class="p">;</span> <span class="n">nHost</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC188'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC189'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// We should be phasing out our use of sites like these.  If we need</span></div><div class='line' id='LC190'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// replacements, we should ask for volunteers to put this simple</span></div><div class='line' id='LC191'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// php file on their webserver that prints the client IP:</span></div><div class='line' id='LC192'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//  &lt;?php echo $_SERVER[&quot;REMOTE_ADDR&quot;]; ?&gt;</span></div><div class='line' id='LC193'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nHost</span> <span class="o">==</span> <span class="mi">1</span><span class="p">)</span></div><div class='line' id='LC194'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC195'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrConnect</span> <span class="o">=</span> <span class="n">CAddress</span><span class="p">(</span><span class="s">&quot;91.198.22.70:80&quot;</span><span class="p">);</span> <span class="c1">// checkip.dyndns.org</span></div><div class='line' id='LC196'><br/></div><div class='line' id='LC197'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nLookup</span> <span class="o">==</span> <span class="mi">1</span><span class="p">)</span></div><div class='line' id='LC198'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC199'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">hostent</span><span class="o">*</span> <span class="n">phostent</span> <span class="o">=</span> <span class="n">gethostbyname</span><span class="p">(</span><span class="s">&quot;checkip.dyndns.org&quot;</span><span class="p">);</span></div><div class='line' id='LC200'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">phostent</span> <span class="o">&amp;&amp;</span> <span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span> <span class="o">&amp;&amp;</span> <span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span><span class="p">[</span><span class="mi">0</span><span class="p">])</span></div><div class='line' id='LC201'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrConnect</span> <span class="o">=</span> <span class="n">CAddress</span><span class="p">(</span><span class="o">*</span><span class="p">(</span><span class="n">u_long</span><span class="o">*</span><span class="p">)</span><span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span><span class="p">[</span><span class="mi">0</span><span class="p">],</span> <span class="n">htons</span><span class="p">(</span><span class="mi">80</span><span class="p">));</span></div><div class='line' id='LC202'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC203'><br/></div><div class='line' id='LC204'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pszGet</span> <span class="o">=</span> <span class="s">&quot;GET / HTTP/1.1</span><span class="se">\r\n</span><span class="s">&quot;</span></div><div class='line' id='LC205'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;Host: checkip.dyndns.org</span><span class="se">\r\n</span><span class="s">&quot;</span></div><div class='line' id='LC206'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)</span><span class="se">\r\n</span><span class="s">&quot;</span></div><div class='line' id='LC207'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;Connection: close</span><span class="se">\r\n</span><span class="s">&quot;</span></div><div class='line' id='LC208'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;</span><span class="se">\r\n</span><span class="s">&quot;</span><span class="p">;</span></div><div class='line' id='LC209'><br/></div><div class='line' id='LC210'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pszKeyword</span> <span class="o">=</span> <span class="s">&quot;Address:&quot;</span><span class="p">;</span></div><div class='line' id='LC211'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC212'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span> <span class="k">if</span> <span class="p">(</span><span class="n">nHost</span> <span class="o">==</span> <span class="mi">2</span><span class="p">)</span></div><div class='line' id='LC213'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC214'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrConnect</span> <span class="o">=</span> <span class="n">CAddress</span><span class="p">(</span><span class="s">&quot;74.208.43.192:80&quot;</span><span class="p">);</span> <span class="c1">// www.showmyip.com</span></div><div class='line' id='LC215'><br/></div><div class='line' id='LC216'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nLookup</span> <span class="o">==</span> <span class="mi">1</span><span class="p">)</span></div><div class='line' id='LC217'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC218'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">hostent</span><span class="o">*</span> <span class="n">phostent</span> <span class="o">=</span> <span class="n">gethostbyname</span><span class="p">(</span><span class="s">&quot;www.showmyip.com&quot;</span><span class="p">);</span></div><div class='line' id='LC219'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">phostent</span> <span class="o">&amp;&amp;</span> <span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span> <span class="o">&amp;&amp;</span> <span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span><span class="p">[</span><span class="mi">0</span><span class="p">])</span></div><div class='line' id='LC220'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrConnect</span> <span class="o">=</span> <span class="n">CAddress</span><span class="p">(</span><span class="o">*</span><span class="p">(</span><span class="n">u_long</span><span class="o">*</span><span class="p">)</span><span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span><span class="p">[</span><span class="mi">0</span><span class="p">],</span> <span class="n">htons</span><span class="p">(</span><span class="mi">80</span><span class="p">));</span></div><div class='line' id='LC221'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC222'><br/></div><div class='line' id='LC223'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pszGet</span> <span class="o">=</span> <span class="s">&quot;GET /simple/ HTTP/1.1</span><span class="se">\r\n</span><span class="s">&quot;</span></div><div class='line' id='LC224'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;Host: www.showmyip.com</span><span class="se">\r\n</span><span class="s">&quot;</span></div><div class='line' id='LC225'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)</span><span class="se">\r\n</span><span class="s">&quot;</span></div><div class='line' id='LC226'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;Connection: close</span><span class="se">\r\n</span><span class="s">&quot;</span></div><div class='line' id='LC227'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="s">&quot;</span><span class="se">\r\n</span><span class="s">&quot;</span><span class="p">;</span></div><div class='line' id='LC228'><br/></div><div class='line' id='LC229'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pszKeyword</span> <span class="o">=</span> <span class="nb">NULL</span><span class="p">;</span> <span class="c1">// Returns just IP address</span></div><div class='line' id='LC230'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC231'><br/></div><div class='line' id='LC232'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetMyExternalIP2</span><span class="p">(</span><span class="n">addrConnect</span><span class="p">,</span> <span class="n">pszGet</span><span class="p">,</span> <span class="n">pszKeyword</span><span class="p">,</span> <span class="n">ipRet</span><span class="p">))</span></div><div class='line' id='LC233'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC234'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC235'><br/></div><div class='line' id='LC236'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC237'><span class="p">}</span></div><div class='line' id='LC238'><br/></div><div class='line' id='LC239'><span class="kt">void</span> <span class="n">ThreadGetMyExternalIP</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC240'><span class="p">{</span></div><div class='line' id='LC241'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Wait for IRC to get it first</span></div><div class='line' id='LC242'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-noirc&quot;</span><span class="p">))</span></div><div class='line' id='LC243'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC244'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">i</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="n">i</span> <span class="o">&lt;</span> <span class="mi">2</span> <span class="o">*</span> <span class="mi">60</span><span class="p">;</span> <span class="n">i</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC245'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC246'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">1000</span><span class="p">);</span></div><div class='line' id='LC247'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fGotExternalIP</span> <span class="o">||</span> <span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC248'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC249'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC250'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC251'><br/></div><div class='line' id='LC252'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Fallback in case IRC fails to get it</span></div><div class='line' id='LC253'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetMyExternalIP</span><span class="p">(</span><span class="n">addrLocalHost</span><span class="p">.</span><span class="n">ip</span><span class="p">))</span></div><div class='line' id='LC254'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC255'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;GetMyExternalIP() returned %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">addrLocalHost</span><span class="p">.</span><span class="n">ToStringIP</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC256'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addrLocalHost</span><span class="p">.</span><span class="n">IsRoutable</span><span class="p">())</span></div><div class='line' id='LC257'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC258'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// If we already connected to a few before we had our IP, go back and addr them.</span></div><div class='line' id='LC259'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// setAddrKnown automatically filters any duplicate sends.</span></div><div class='line' id='LC260'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addr</span><span class="p">(</span><span class="n">addrLocalHost</span><span class="p">);</span></div><div class='line' id='LC261'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addr</span><span class="p">.</span><span class="n">nTime</span> <span class="o">=</span> <span class="n">GetAdjustedTime</span><span class="p">();</span></div><div class='line' id='LC262'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC263'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC264'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">PushAddress</span><span class="p">(</span><span class="n">addr</span><span class="p">);</span></div><div class='line' id='LC265'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC266'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC267'><span class="p">}</span></div><div class='line' id='LC268'><br/></div><div class='line' id='LC269'><br/></div><div class='line' id='LC270'><br/></div><div class='line' id='LC271'><br/></div><div class='line' id='LC272'><br/></div><div class='line' id='LC273'><span class="kt">bool</span> <span class="n">AddAddress</span><span class="p">(</span><span class="n">CAddress</span> <span class="n">addr</span><span class="p">,</span> <span class="n">int64</span> <span class="n">nTimePenalty</span><span class="p">)</span></div><div class='line' id='LC274'><span class="p">{</span></div><div class='line' id='LC275'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">addr</span><span class="p">.</span><span class="n">IsRoutable</span><span class="p">())</span></div><div class='line' id='LC276'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC277'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">ip</span> <span class="o">==</span> <span class="n">addrLocalHost</span><span class="p">.</span><span class="n">ip</span><span class="p">)</span></div><div class='line' id='LC278'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC279'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addr</span><span class="p">.</span><span class="n">nTime</span> <span class="o">=</span> <span class="n">max</span><span class="p">((</span><span class="n">int64</span><span class="p">)</span><span class="mi">0</span><span class="p">,</span> <span class="p">(</span><span class="n">int64</span><span class="p">)</span><span class="n">addr</span><span class="p">.</span><span class="n">nTime</span> <span class="o">-</span> <span class="n">nTimePenalty</span><span class="p">);</span></div><div class='line' id='LC280'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_mapAddresses</span><span class="p">)</span></div><div class='line' id='LC281'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC282'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">map</span><span class="o">&lt;</span><span class="n">vector</span><span class="o">&lt;</span><span class="kt">unsigned</span> <span class="kt">char</span><span class="o">&gt;</span><span class="p">,</span> <span class="n">CAddress</span><span class="o">&gt;::</span><span class="n">iterator</span> <span class="n">it</span> <span class="o">=</span> <span class="n">mapAddresses</span><span class="p">.</span><span class="n">find</span><span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">GetKey</span><span class="p">());</span></div><div class='line' id='LC283'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">it</span> <span class="o">==</span> <span class="n">mapAddresses</span><span class="p">.</span><span class="n">end</span><span class="p">())</span></div><div class='line' id='LC284'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC285'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// New address</span></div><div class='line' id='LC286'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;AddAddress(%s)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">addr</span><span class="p">.</span><span class="n">ToStringLog</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC287'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">mapAddresses</span><span class="p">.</span><span class="n">insert</span><span class="p">(</span><span class="n">make_pair</span><span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">GetKey</span><span class="p">(),</span> <span class="n">addr</span><span class="p">));</span></div><div class='line' id='LC288'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddrDB</span><span class="p">().</span><span class="n">WriteAddress</span><span class="p">(</span><span class="n">addr</span><span class="p">);</span></div><div class='line' id='LC289'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC290'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC291'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC292'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC293'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">bool</span> <span class="n">fUpdated</span> <span class="o">=</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC294'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span><span class="o">&amp;</span> <span class="n">addrFound</span> <span class="o">=</span> <span class="p">(</span><span class="o">*</span><span class="n">it</span><span class="p">).</span><span class="n">second</span><span class="p">;</span></div><div class='line' id='LC295'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">((</span><span class="n">addrFound</span><span class="p">.</span><span class="n">nServices</span> <span class="o">|</span> <span class="n">addr</span><span class="p">.</span><span class="n">nServices</span><span class="p">)</span> <span class="o">!=</span> <span class="n">addrFound</span><span class="p">.</span><span class="n">nServices</span><span class="p">)</span></div><div class='line' id='LC296'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC297'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Services have been added</span></div><div class='line' id='LC298'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrFound</span><span class="p">.</span><span class="n">nServices</span> <span class="o">|=</span> <span class="n">addr</span><span class="p">.</span><span class="n">nServices</span><span class="p">;</span></div><div class='line' id='LC299'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fUpdated</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC300'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC301'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">bool</span> <span class="n">fCurrentlyOnline</span> <span class="o">=</span> <span class="p">(</span><span class="n">GetAdjustedTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">addr</span><span class="p">.</span><span class="n">nTime</span> <span class="o">&lt;</span> <span class="mi">24</span> <span class="o">*</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span><span class="p">);</span></div><div class='line' id='LC302'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nUpdateInterval</span> <span class="o">=</span> <span class="p">(</span><span class="n">fCurrentlyOnline</span> <span class="o">?</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span> <span class="o">:</span> <span class="mi">24</span> <span class="o">*</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span><span class="p">);</span></div><div class='line' id='LC303'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addrFound</span><span class="p">.</span><span class="n">nTime</span> <span class="o">&lt;</span> <span class="n">addr</span><span class="p">.</span><span class="n">nTime</span> <span class="o">-</span> <span class="n">nUpdateInterval</span><span class="p">)</span></div><div class='line' id='LC304'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC305'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Periodically update most recently seen time</span></div><div class='line' id='LC306'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrFound</span><span class="p">.</span><span class="n">nTime</span> <span class="o">=</span> <span class="n">addr</span><span class="p">.</span><span class="n">nTime</span><span class="p">;</span></div><div class='line' id='LC307'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fUpdated</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC308'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC309'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fUpdated</span><span class="p">)</span></div><div class='line' id='LC310'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddrDB</span><span class="p">().</span><span class="n">WriteAddress</span><span class="p">(</span><span class="n">addrFound</span><span class="p">);</span></div><div class='line' id='LC311'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC312'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC313'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC314'><span class="p">}</span></div><div class='line' id='LC315'><br/></div><div class='line' id='LC316'><span class="kt">void</span> <span class="n">AddressCurrentlyConnected</span><span class="p">(</span><span class="k">const</span> <span class="n">CAddress</span><span class="o">&amp;</span> <span class="n">addr</span><span class="p">)</span></div><div class='line' id='LC317'><span class="p">{</span></div><div class='line' id='LC318'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_mapAddresses</span><span class="p">)</span></div><div class='line' id='LC319'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC320'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Only if it&#39;s been published already</span></div><div class='line' id='LC321'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">map</span><span class="o">&lt;</span><span class="n">vector</span><span class="o">&lt;</span><span class="kt">unsigned</span> <span class="kt">char</span><span class="o">&gt;</span><span class="p">,</span> <span class="n">CAddress</span><span class="o">&gt;::</span><span class="n">iterator</span> <span class="n">it</span> <span class="o">=</span> <span class="n">mapAddresses</span><span class="p">.</span><span class="n">find</span><span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">GetKey</span><span class="p">());</span></div><div class='line' id='LC322'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">it</span> <span class="o">!=</span> <span class="n">mapAddresses</span><span class="p">.</span><span class="n">end</span><span class="p">())</span></div><div class='line' id='LC323'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC324'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span><span class="o">&amp;</span> <span class="n">addrFound</span> <span class="o">=</span> <span class="p">(</span><span class="o">*</span><span class="n">it</span><span class="p">).</span><span class="n">second</span><span class="p">;</span></div><div class='line' id='LC325'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nUpdateInterval</span> <span class="o">=</span> <span class="mi">20</span> <span class="o">*</span> <span class="mi">60</span><span class="p">;</span></div><div class='line' id='LC326'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addrFound</span><span class="p">.</span><span class="n">nTime</span> <span class="o">&lt;</span> <span class="n">GetAdjustedTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">nUpdateInterval</span><span class="p">)</span></div><div class='line' id='LC327'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC328'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Periodically update most recently seen time</span></div><div class='line' id='LC329'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrFound</span><span class="p">.</span><span class="n">nTime</span> <span class="o">=</span> <span class="n">GetAdjustedTime</span><span class="p">();</span></div><div class='line' id='LC330'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddrDB</span> <span class="n">addrdb</span><span class="p">;</span></div><div class='line' id='LC331'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrdb</span><span class="p">.</span><span class="n">WriteAddress</span><span class="p">(</span><span class="n">addrFound</span><span class="p">);</span></div><div class='line' id='LC332'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC333'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC334'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC335'><span class="p">}</span></div><div class='line' id='LC336'><br/></div><div class='line' id='LC337'><br/></div><div class='line' id='LC338'><br/></div><div class='line' id='LC339'><br/></div><div class='line' id='LC340'><br/></div><div class='line' id='LC341'><span class="kt">void</span> <span class="n">AbandonRequests</span><span class="p">(</span><span class="kt">void</span> <span class="p">(</span><span class="o">*</span><span class="n">fn</span><span class="p">)(</span><span class="kt">void</span><span class="o">*</span><span class="p">,</span> <span class="n">CDataStream</span><span class="o">&amp;</span><span class="p">),</span> <span class="kt">void</span><span class="o">*</span> <span class="n">param1</span><span class="p">)</span></div><div class='line' id='LC342'><span class="p">{</span></div><div class='line' id='LC343'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// If the dialog might get closed before the reply comes back,</span></div><div class='line' id='LC344'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// call this in the destructor so it doesn&#39;t get called after it&#39;s deleted.</span></div><div class='line' id='LC345'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC346'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC347'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC348'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC349'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_mapRequests</span><span class="p">)</span></div><div class='line' id='LC350'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC351'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="n">map</span><span class="o">&lt;</span><span class="n">uint256</span><span class="p">,</span> <span class="n">CRequestTracker</span><span class="o">&gt;::</span><span class="n">iterator</span> <span class="n">mi</span> <span class="o">=</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">mapRequests</span><span class="p">.</span><span class="n">begin</span><span class="p">();</span> <span class="n">mi</span> <span class="o">!=</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">mapRequests</span><span class="p">.</span><span class="n">end</span><span class="p">();)</span></div><div class='line' id='LC352'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC353'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRequestTracker</span><span class="o">&amp;</span> <span class="n">tracker</span> <span class="o">=</span> <span class="p">(</span><span class="o">*</span><span class="n">mi</span><span class="p">).</span><span class="n">second</span><span class="p">;</span></div><div class='line' id='LC354'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">tracker</span><span class="p">.</span><span class="n">fn</span> <span class="o">==</span> <span class="n">fn</span> <span class="o">&amp;&amp;</span> <span class="n">tracker</span><span class="p">.</span><span class="n">param1</span> <span class="o">==</span> <span class="n">param1</span><span class="p">)</span></div><div class='line' id='LC355'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">mapRequests</span><span class="p">.</span><span class="n">erase</span><span class="p">(</span><span class="n">mi</span><span class="o">++</span><span class="p">);</span></div><div class='line' id='LC356'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC357'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">mi</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC358'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC359'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC360'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC361'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC362'><span class="p">}</span></div><div class='line' id='LC363'><br/></div><div class='line' id='LC364'><br/></div><div class='line' id='LC365'><br/></div><div class='line' id='LC366'><br/></div><div class='line' id='LC367'><br/></div><div class='line' id='LC368'><br/></div><div class='line' id='LC369'><br/></div><div class='line' id='LC370'><span class="c1">//</span></div><div class='line' id='LC371'><span class="c1">// Subscription methods for the broadcast and subscription system.</span></div><div class='line' id='LC372'><span class="c1">// Channel numbers are message numbers, i.e. MSG_TABLE and MSG_PRODUCT.</span></div><div class='line' id='LC373'><span class="c1">//</span></div><div class='line' id='LC374'><span class="c1">// The subscription system uses a meet-in-the-middle strategy.</span></div><div class='line' id='LC375'><span class="c1">// With 100,000 nodes, if senders broadcast to 1000 random nodes and receivers</span></div><div class='line' id='LC376'><span class="c1">// subscribe to 1000 random nodes, 99.995% (1 - 0.99^1000) of messages will get through.</span></div><div class='line' id='LC377'><span class="c1">//</span></div><div class='line' id='LC378'><br/></div><div class='line' id='LC379'><span class="kt">bool</span> <span class="n">AnySubscribed</span><span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">nChannel</span><span class="p">)</span></div><div class='line' id='LC380'><span class="p">{</span></div><div class='line' id='LC381'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnodeLocalHost</span><span class="o">-&gt;</span><span class="n">IsSubscribed</span><span class="p">(</span><span class="n">nChannel</span><span class="p">))</span></div><div class='line' id='LC382'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC383'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC384'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC385'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">IsSubscribed</span><span class="p">(</span><span class="n">nChannel</span><span class="p">))</span></div><div class='line' id='LC386'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC387'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC388'><span class="p">}</span></div><div class='line' id='LC389'><br/></div><div class='line' id='LC390'><span class="kt">bool</span> <span class="n">CNode</span><span class="o">::</span><span class="n">IsSubscribed</span><span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">nChannel</span><span class="p">)</span></div><div class='line' id='LC391'><span class="p">{</span></div><div class='line' id='LC392'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nChannel</span> <span class="o">&gt;=</span> <span class="n">vfSubscribe</span><span class="p">.</span><span class="n">size</span><span class="p">())</span></div><div class='line' id='LC393'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC394'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="n">vfSubscribe</span><span class="p">[</span><span class="n">nChannel</span><span class="p">];</span></div><div class='line' id='LC395'><span class="p">}</span></div><div class='line' id='LC396'><br/></div><div class='line' id='LC397'><span class="kt">void</span> <span class="n">CNode</span><span class="o">::</span><span class="n">Subscribe</span><span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">nChannel</span><span class="p">,</span> <span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">nHops</span><span class="p">)</span></div><div class='line' id='LC398'><span class="p">{</span></div><div class='line' id='LC399'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nChannel</span> <span class="o">&gt;=</span> <span class="n">vfSubscribe</span><span class="p">.</span><span class="n">size</span><span class="p">())</span></div><div class='line' id='LC400'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC401'><br/></div><div class='line' id='LC402'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">AnySubscribed</span><span class="p">(</span><span class="n">nChannel</span><span class="p">))</span></div><div class='line' id='LC403'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC404'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Relay subscribe</span></div><div class='line' id='LC405'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC406'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC407'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span> <span class="o">!=</span> <span class="k">this</span><span class="p">)</span></div><div class='line' id='LC408'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">PushMessage</span><span class="p">(</span><span class="s">&quot;subscribe&quot;</span><span class="p">,</span> <span class="n">nChannel</span><span class="p">,</span> <span class="n">nHops</span><span class="p">);</span></div><div class='line' id='LC409'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC410'><br/></div><div class='line' id='LC411'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vfSubscribe</span><span class="p">[</span><span class="n">nChannel</span><span class="p">]</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC412'><span class="p">}</span></div><div class='line' id='LC413'><br/></div><div class='line' id='LC414'><span class="kt">void</span> <span class="n">CNode</span><span class="o">::</span><span class="n">CancelSubscribe</span><span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">nChannel</span><span class="p">)</span></div><div class='line' id='LC415'><span class="p">{</span></div><div class='line' id='LC416'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nChannel</span> <span class="o">&gt;=</span> <span class="n">vfSubscribe</span><span class="p">.</span><span class="n">size</span><span class="p">())</span></div><div class='line' id='LC417'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC418'><br/></div><div class='line' id='LC419'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Prevent from relaying cancel if wasn&#39;t subscribed</span></div><div class='line' id='LC420'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">vfSubscribe</span><span class="p">[</span><span class="n">nChannel</span><span class="p">])</span></div><div class='line' id='LC421'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC422'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vfSubscribe</span><span class="p">[</span><span class="n">nChannel</span><span class="p">]</span> <span class="o">=</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC423'><br/></div><div class='line' id='LC424'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">AnySubscribed</span><span class="p">(</span><span class="n">nChannel</span><span class="p">))</span></div><div class='line' id='LC425'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC426'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Relay subscription cancel</span></div><div class='line' id='LC427'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC428'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC429'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span> <span class="o">!=</span> <span class="k">this</span><span class="p">)</span></div><div class='line' id='LC430'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">PushMessage</span><span class="p">(</span><span class="s">&quot;sub-cancel&quot;</span><span class="p">,</span> <span class="n">nChannel</span><span class="p">);</span></div><div class='line' id='LC431'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC432'><span class="p">}</span></div><div class='line' id='LC433'><br/></div><div class='line' id='LC434'><br/></div><div class='line' id='LC435'><br/></div><div class='line' id='LC436'><br/></div><div class='line' id='LC437'><br/></div><div class='line' id='LC438'><br/></div><div class='line' id='LC439'><br/></div><div class='line' id='LC440'><br/></div><div class='line' id='LC441'><br/></div><div class='line' id='LC442'><span class="n">CNode</span><span class="o">*</span> <span class="n">FindNode</span><span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">ip</span><span class="p">)</span></div><div class='line' id='LC443'><span class="p">{</span></div><div class='line' id='LC444'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC445'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC446'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC447'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">addr</span><span class="p">.</span><span class="n">ip</span> <span class="o">==</span> <span class="n">ip</span><span class="p">)</span></div><div class='line' id='LC448'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="p">(</span><span class="n">pnode</span><span class="p">);</span></div><div class='line' id='LC449'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC450'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="nb">NULL</span><span class="p">;</span></div><div class='line' id='LC451'><span class="p">}</span></div><div class='line' id='LC452'><br/></div><div class='line' id='LC453'><span class="n">CNode</span><span class="o">*</span> <span class="n">FindNode</span><span class="p">(</span><span class="n">CAddress</span> <span class="n">addr</span><span class="p">)</span></div><div class='line' id='LC454'><span class="p">{</span></div><div class='line' id='LC455'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC456'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC457'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC458'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">addr</span> <span class="o">==</span> <span class="n">addr</span><span class="p">)</span></div><div class='line' id='LC459'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="p">(</span><span class="n">pnode</span><span class="p">);</span></div><div class='line' id='LC460'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC461'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="nb">NULL</span><span class="p">;</span></div><div class='line' id='LC462'><span class="p">}</span></div><div class='line' id='LC463'><br/></div><div class='line' id='LC464'><span class="n">CNode</span><span class="o">*</span> <span class="n">ConnectNode</span><span class="p">(</span><span class="n">CAddress</span> <span class="n">addrConnect</span><span class="p">,</span> <span class="n">int64</span> <span class="n">nTimeout</span><span class="p">)</span></div><div class='line' id='LC465'><span class="p">{</span></div><div class='line' id='LC466'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">ip</span> <span class="o">==</span> <span class="n">addrLocalHost</span><span class="p">.</span><span class="n">ip</span><span class="p">)</span></div><div class='line' id='LC467'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="nb">NULL</span><span class="p">;</span></div><div class='line' id='LC468'><br/></div><div class='line' id='LC469'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Look for an existing connection</span></div><div class='line' id='LC470'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span> <span class="o">=</span> <span class="n">FindNode</span><span class="p">(</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">ip</span><span class="p">);</span></div><div class='line' id='LC471'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="p">)</span></div><div class='line' id='LC472'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC473'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nTimeout</span> <span class="o">!=</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC474'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">AddRef</span><span class="p">(</span><span class="n">nTimeout</span><span class="p">);</span></div><div class='line' id='LC475'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC476'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">AddRef</span><span class="p">();</span></div><div class='line' id='LC477'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="n">pnode</span><span class="p">;</span></div><div class='line' id='LC478'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC479'><br/></div><div class='line' id='LC480'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">/// debug print</span></div><div class='line' id='LC481'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;trying connection %s lastseen=%.1fhrs lasttry=%.1fhrs</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span></div><div class='line' id='LC482'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrConnect</span><span class="p">.</span><span class="n">ToStringLog</span><span class="p">().</span><span class="n">c_str</span><span class="p">(),</span></div><div class='line' id='LC483'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">(</span><span class="kt">double</span><span class="p">)(</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">nTime</span> <span class="o">-</span> <span class="n">GetAdjustedTime</span><span class="p">())</span><span class="o">/</span><span class="mf">3600.0</span><span class="p">,</span></div><div class='line' id='LC484'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">(</span><span class="kt">double</span><span class="p">)(</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">nLastTry</span> <span class="o">-</span> <span class="n">GetAdjustedTime</span><span class="p">())</span><span class="o">/</span><span class="mf">3600.0</span><span class="p">);</span></div><div class='line' id='LC485'><br/></div><div class='line' id='LC486'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_mapAddresses</span><span class="p">)</span></div><div class='line' id='LC487'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">mapAddresses</span><span class="p">[</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">GetKey</span><span class="p">()].</span><span class="n">nLastTry</span> <span class="o">=</span> <span class="n">GetAdjustedTime</span><span class="p">();</span></div><div class='line' id='LC488'><br/></div><div class='line' id='LC489'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Connect</span></div><div class='line' id='LC490'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">SOCKET</span> <span class="n">hSocket</span><span class="p">;</span></div><div class='line' id='LC491'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">ConnectSocket</span><span class="p">(</span><span class="n">addrConnect</span><span class="p">,</span> <span class="n">hSocket</span><span class="p">))</span></div><div class='line' id='LC492'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC493'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">/// debug print</span></div><div class='line' id='LC494'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;connected %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">addrConnect</span><span class="p">.</span><span class="n">ToStringLog</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC495'><br/></div><div class='line' id='LC496'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Set to nonblocking</span></div><div class='line' id='LC497'><span class="cp">#ifdef __WXMSW__</span></div><div class='line' id='LC498'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">u_long</span> <span class="n">nOne</span> <span class="o">=</span> <span class="mi">1</span><span class="p">;</span></div><div class='line' id='LC499'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">ioctlsocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">FIONBIO</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">nOne</span><span class="p">)</span> <span class="o">==</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC500'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ConnectSocket() : ioctlsocket nonblocking setting failed, error %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">WSAGetLastError</span><span class="p">());</span></div><div class='line' id='LC501'><span class="cp">#else</span></div><div class='line' id='LC502'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fcntl</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">F_SETFL</span><span class="p">,</span> <span class="n">O_NONBLOCK</span><span class="p">)</span> <span class="o">==</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC503'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ConnectSocket() : fcntl nonblocking setting failed, error %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">errno</span><span class="p">);</span></div><div class='line' id='LC504'><span class="cp">#endif</span></div><div class='line' id='LC505'><br/></div><div class='line' id='LC506'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Add node</span></div><div class='line' id='LC507'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span> <span class="o">=</span> <span class="k">new</span> <span class="n">CNode</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">addrConnect</span><span class="p">,</span> <span class="kc">false</span><span class="p">);</span></div><div class='line' id='LC508'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nTimeout</span> <span class="o">!=</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC509'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">AddRef</span><span class="p">(</span><span class="n">nTimeout</span><span class="p">);</span></div><div class='line' id='LC510'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC511'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">AddRef</span><span class="p">();</span></div><div class='line' id='LC512'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC513'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vNodes</span><span class="p">.</span><span class="n">push_back</span><span class="p">(</span><span class="n">pnode</span><span class="p">);</span></div><div class='line' id='LC514'><br/></div><div class='line' id='LC515'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nTimeConnected</span> <span class="o">=</span> <span class="n">GetTime</span><span class="p">();</span></div><div class='line' id='LC516'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="n">pnode</span><span class="p">;</span></div><div class='line' id='LC517'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC518'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC519'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC520'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="nb">NULL</span><span class="p">;</span></div><div class='line' id='LC521'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC522'><span class="p">}</span></div><div class='line' id='LC523'><br/></div><div class='line' id='LC524'><span class="kt">void</span> <span class="n">CNode</span><span class="o">::</span><span class="n">CloseSocketDisconnect</span><span class="p">()</span></div><div class='line' id='LC525'><span class="p">{</span></div><div class='line' id='LC526'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fDisconnect</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC527'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">hSocket</span> <span class="o">!=</span> <span class="n">INVALID_SOCKET</span><span class="p">)</span></div><div class='line' id='LC528'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC529'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fDebug</span><span class="p">)</span></div><div class='line' id='LC530'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;%s &quot;</span><span class="p">,</span> <span class="n">DateTimeStrFormat</span><span class="p">(</span><span class="s">&quot;%x %H:%M:%S&quot;</span><span class="p">,</span> <span class="n">GetTime</span><span class="p">()).</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC531'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;disconnecting node %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">addr</span><span class="p">.</span><span class="n">ToStringLog</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC532'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC533'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">hSocket</span> <span class="o">=</span> <span class="n">INVALID_SOCKET</span><span class="p">;</span></div><div class='line' id='LC534'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC535'><span class="p">}</span></div><div class='line' id='LC536'><br/></div><div class='line' id='LC537'><span class="kt">void</span> <span class="n">CNode</span><span class="o">::</span><span class="n">Cleanup</span><span class="p">()</span></div><div class='line' id='LC538'><span class="p">{</span></div><div class='line' id='LC539'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// All of a nodes broadcasts and subscriptions are automatically torn down</span></div><div class='line' id='LC540'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// when it goes down, so a node has to stay up to keep its broadcast going.</span></div><div class='line' id='LC541'><br/></div><div class='line' id='LC542'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Cancel subscriptions</span></div><div class='line' id='LC543'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">nChannel</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="n">nChannel</span> <span class="o">&lt;</span> <span class="n">vfSubscribe</span><span class="p">.</span><span class="n">size</span><span class="p">();</span> <span class="n">nChannel</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC544'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vfSubscribe</span><span class="p">[</span><span class="n">nChannel</span><span class="p">])</span></div><div class='line' id='LC545'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CancelSubscribe</span><span class="p">(</span><span class="n">nChannel</span><span class="p">);</span></div><div class='line' id='LC546'><span class="p">}</span></div><div class='line' id='LC547'><br/></div><div class='line' id='LC548'><br/></div><div class='line' id='LC549'><br/></div><div class='line' id='LC550'><br/></div><div class='line' id='LC551'><br/></div><div class='line' id='LC552'><br/></div><div class='line' id='LC553'><br/></div><div class='line' id='LC554'><br/></div><div class='line' id='LC555'><br/></div><div class='line' id='LC556'><br/></div><div class='line' id='LC557'><br/></div><div class='line' id='LC558'><br/></div><div class='line' id='LC559'><br/></div><div class='line' id='LC560'><span class="kt">void</span> <span class="n">ThreadSocketHandler</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC561'><span class="p">{</span></div><div class='line' id='LC562'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">IMPLEMENT_RANDOMIZE_STACK</span><span class="p">(</span><span class="n">ThreadSocketHandler</span><span class="p">(</span><span class="n">parg</span><span class="p">));</span></div><div class='line' id='LC563'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">try</span></div><div class='line' id='LC564'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC565'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">0</span><span class="p">]</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC566'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ThreadSocketHandler2</span><span class="p">(</span><span class="n">parg</span><span class="p">);</span></div><div class='line' id='LC567'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">0</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC568'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC569'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">catch</span> <span class="p">(</span><span class="n">std</span><span class="o">::</span><span class="n">exception</span><span class="o">&amp;</span> <span class="n">e</span><span class="p">)</span> <span class="p">{</span></div><div class='line' id='LC570'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">0</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC571'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintException</span><span class="p">(</span><span class="o">&amp;</span><span class="n">e</span><span class="p">,</span> <span class="s">&quot;ThreadSocketHandler()&quot;</span><span class="p">);</span></div><div class='line' id='LC572'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span> <span class="k">catch</span> <span class="p">(...)</span> <span class="p">{</span></div><div class='line' id='LC573'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">0</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC574'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">throw</span><span class="p">;</span> <span class="c1">// support pthread_cancel()</span></div><div class='line' id='LC575'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC576'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadSocketHandler exiting</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC577'><span class="p">}</span></div><div class='line' id='LC578'><br/></div><div class='line' id='LC579'><span class="kt">void</span> <span class="n">ThreadSocketHandler2</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC580'><span class="p">{</span></div><div class='line' id='LC581'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadSocketHandler started</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC582'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">list</span><span class="o">&lt;</span><span class="n">CNode</span><span class="o">*&gt;</span> <span class="n">vNodesDisconnected</span><span class="p">;</span></div><div class='line' id='LC583'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nPrevNodeCount</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC584'><br/></div><div class='line' id='LC585'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">loop</span></div><div class='line' id='LC586'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC587'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC588'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Disconnect nodes</span></div><div class='line' id='LC589'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC590'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC591'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC592'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Disconnect unused nodes</span></div><div class='line' id='LC593'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vector</span><span class="o">&lt;</span><span class="n">CNode</span><span class="o">*&gt;</span> <span class="n">vNodesCopy</span> <span class="o">=</span> <span class="n">vNodes</span><span class="p">;</span></div><div class='line' id='LC594'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodesCopy</span><span class="p">)</span></div><div class='line' id='LC595'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC596'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fDisconnect</span> <span class="o">||</span></div><div class='line' id='LC597'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">GetRefCount</span><span class="p">()</span> <span class="o">&lt;=</span> <span class="mi">0</span> <span class="o">&amp;&amp;</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">vRecv</span><span class="p">.</span><span class="n">empty</span><span class="p">()</span> <span class="o">&amp;&amp;</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">vSend</span><span class="p">.</span><span class="n">empty</span><span class="p">()))</span></div><div class='line' id='LC598'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC599'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// remove from vNodes</span></div><div class='line' id='LC600'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vNodes</span><span class="p">.</span><span class="n">erase</span><span class="p">(</span><span class="n">remove</span><span class="p">(</span><span class="n">vNodes</span><span class="p">.</span><span class="n">begin</span><span class="p">(),</span> <span class="n">vNodes</span><span class="p">.</span><span class="n">end</span><span class="p">(),</span> <span class="n">pnode</span><span class="p">),</span> <span class="n">vNodes</span><span class="p">.</span><span class="n">end</span><span class="p">());</span></div><div class='line' id='LC601'><br/></div><div class='line' id='LC602'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// close socket and cleanup</span></div><div class='line' id='LC603'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">CloseSocketDisconnect</span><span class="p">();</span></div><div class='line' id='LC604'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">Cleanup</span><span class="p">();</span></div><div class='line' id='LC605'><br/></div><div class='line' id='LC606'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// hold in disconnected pool until all refs are released</span></div><div class='line' id='LC607'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nReleaseTime</span> <span class="o">=</span> <span class="n">max</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nReleaseTime</span><span class="p">,</span> <span class="n">GetTime</span><span class="p">()</span> <span class="o">+</span> <span class="mi">15</span> <span class="o">*</span> <span class="mi">60</span><span class="p">);</span></div><div class='line' id='LC608'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fNetworkNode</span> <span class="o">||</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fInbound</span><span class="p">)</span></div><div class='line' id='LC609'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">Release</span><span class="p">();</span></div><div class='line' id='LC610'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vNodesDisconnected</span><span class="p">.</span><span class="n">push_back</span><span class="p">(</span><span class="n">pnode</span><span class="p">);</span></div><div class='line' id='LC611'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC612'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC613'><br/></div><div class='line' id='LC614'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Delete disconnected nodes</span></div><div class='line' id='LC615'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">list</span><span class="o">&lt;</span><span class="n">CNode</span><span class="o">*&gt;</span> <span class="n">vNodesDisconnectedCopy</span> <span class="o">=</span> <span class="n">vNodesDisconnected</span><span class="p">;</span></div><div class='line' id='LC616'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodesDisconnectedCopy</span><span class="p">)</span></div><div class='line' id='LC617'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC618'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// wait until threads are done using it</span></div><div class='line' id='LC619'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">GetRefCount</span><span class="p">()</span> <span class="o">&lt;=</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC620'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC621'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">bool</span> <span class="n">fDelete</span> <span class="o">=</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC622'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">TRY_CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_vSend</span><span class="p">)</span></div><div class='line' id='LC623'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">TRY_CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_vRecv</span><span class="p">)</span></div><div class='line' id='LC624'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">TRY_CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_mapRequests</span><span class="p">)</span></div><div class='line' id='LC625'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">TRY_CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_inventory</span><span class="p">)</span></div><div class='line' id='LC626'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fDelete</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC627'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fDelete</span><span class="p">)</span></div><div class='line' id='LC628'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC629'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vNodesDisconnected</span><span class="p">.</span><span class="n">remove</span><span class="p">(</span><span class="n">pnode</span><span class="p">);</span></div><div class='line' id='LC630'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">delete</span> <span class="n">pnode</span><span class="p">;</span></div><div class='line' id='LC631'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC632'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC633'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC634'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC635'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vNodes</span><span class="p">.</span><span class="n">size</span><span class="p">()</span> <span class="o">!=</span> <span class="n">nPrevNodeCount</span><span class="p">)</span></div><div class='line' id='LC636'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC637'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nPrevNodeCount</span> <span class="o">=</span> <span class="n">vNodes</span><span class="p">.</span><span class="n">size</span><span class="p">();</span></div><div class='line' id='LC638'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">MainFrameRepaint</span><span class="p">();</span></div><div class='line' id='LC639'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC640'><br/></div><div class='line' id='LC641'><br/></div><div class='line' id='LC642'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC643'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Find which sockets have data to receive</span></div><div class='line' id='LC644'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC645'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">timeval</span> <span class="n">timeout</span><span class="p">;</span></div><div class='line' id='LC646'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">timeout</span><span class="p">.</span><span class="n">tv_sec</span>  <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC647'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">timeout</span><span class="p">.</span><span class="n">tv_usec</span> <span class="o">=</span> <span class="mi">50000</span><span class="p">;</span> <span class="c1">// frequency to poll pnode-&gt;vSend</span></div><div class='line' id='LC648'><br/></div><div class='line' id='LC649'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fd_set</span> <span class="n">fdsetRecv</span><span class="p">;</span></div><div class='line' id='LC650'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fd_set</span> <span class="n">fdsetSend</span><span class="p">;</span></div><div class='line' id='LC651'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fd_set</span> <span class="n">fdsetError</span><span class="p">;</span></div><div class='line' id='LC652'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_ZERO</span><span class="p">(</span><span class="o">&amp;</span><span class="n">fdsetRecv</span><span class="p">);</span></div><div class='line' id='LC653'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_ZERO</span><span class="p">(</span><span class="o">&amp;</span><span class="n">fdsetSend</span><span class="p">);</span></div><div class='line' id='LC654'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_ZERO</span><span class="p">(</span><span class="o">&amp;</span><span class="n">fdsetError</span><span class="p">);</span></div><div class='line' id='LC655'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">SOCKET</span> <span class="n">hSocketMax</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC656'><br/></div><div class='line' id='LC657'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span><span class="p">(</span><span class="n">hListenSocket</span> <span class="o">!=</span> <span class="n">INVALID_SOCKET</span><span class="p">)</span></div><div class='line' id='LC658'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_SET</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetRecv</span><span class="p">);</span></div><div class='line' id='LC659'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">hSocketMax</span> <span class="o">=</span> <span class="n">max</span><span class="p">(</span><span class="n">hSocketMax</span><span class="p">,</span> <span class="n">hListenSocket</span><span class="p">);</span></div><div class='line' id='LC660'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC661'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC662'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC663'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC664'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span> <span class="o">==</span> <span class="n">INVALID_SOCKET</span> <span class="o">||</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span> <span class="o">&lt;</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC665'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC666'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_SET</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetRecv</span><span class="p">);</span></div><div class='line' id='LC667'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_SET</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetError</span><span class="p">);</span></div><div class='line' id='LC668'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">hSocketMax</span> <span class="o">=</span> <span class="n">max</span><span class="p">(</span><span class="n">hSocketMax</span><span class="p">,</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC669'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">TRY_CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_vSend</span><span class="p">)</span></div><div class='line' id='LC670'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">vSend</span><span class="p">.</span><span class="n">empty</span><span class="p">())</span></div><div class='line' id='LC671'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_SET</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetSend</span><span class="p">);</span></div><div class='line' id='LC672'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC673'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC674'><br/></div><div class='line' id='LC675'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">0</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC676'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nSelect</span> <span class="o">=</span> <span class="n">select</span><span class="p">(</span><span class="n">hSocketMax</span> <span class="o">+</span> <span class="mi">1</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetRecv</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetSend</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetError</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">timeout</span><span class="p">);</span></div><div class='line' id='LC677'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">0</span><span class="p">]</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC678'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC679'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC680'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nSelect</span> <span class="o">==</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC681'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC682'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nErr</span> <span class="o">=</span> <span class="n">WSAGetLastError</span><span class="p">();</span></div><div class='line' id='LC683'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket select error %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">nErr</span><span class="p">);</span></div><div class='line' id='LC684'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">i</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="n">i</span> <span class="o">&lt;=</span> <span class="n">hSocketMax</span><span class="p">;</span> <span class="n">i</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC685'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_SET</span><span class="p">(</span><span class="n">i</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetRecv</span><span class="p">);</span></div><div class='line' id='LC686'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_ZERO</span><span class="p">(</span><span class="o">&amp;</span><span class="n">fdsetSend</span><span class="p">);</span></div><div class='line' id='LC687'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FD_ZERO</span><span class="p">(</span><span class="o">&amp;</span><span class="n">fdsetError</span><span class="p">);</span></div><div class='line' id='LC688'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="n">timeout</span><span class="p">.</span><span class="n">tv_usec</span><span class="o">/</span><span class="mi">1000</span><span class="p">);</span></div><div class='line' id='LC689'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC690'><br/></div><div class='line' id='LC691'><br/></div><div class='line' id='LC692'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC693'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Accept new connections</span></div><div class='line' id='LC694'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC695'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">hListenSocket</span> <span class="o">!=</span> <span class="n">INVALID_SOCKET</span> <span class="o">&amp;&amp;</span> <span class="n">FD_ISSET</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetRecv</span><span class="p">))</span></div><div class='line' id='LC696'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC697'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">sockaddr_in</span> <span class="n">sockaddr</span><span class="p">;</span></div><div class='line' id='LC698'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">socklen_t</span> <span class="n">len</span> <span class="o">=</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">sockaddr</span><span class="p">);</span></div><div class='line' id='LC699'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">SOCKET</span> <span class="n">hSocket</span> <span class="o">=</span> <span class="n">accept</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">,</span> <span class="p">(</span><span class="k">struct</span> <span class="n">sockaddr</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="n">sockaddr</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">len</span><span class="p">);</span></div><div class='line' id='LC700'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addr</span><span class="p">(</span><span class="n">sockaddr</span><span class="p">);</span></div><div class='line' id='LC701'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nInbound</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC702'><br/></div><div class='line' id='LC703'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC704'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC705'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fInbound</span><span class="p">)</span></div><div class='line' id='LC706'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nInbound</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC707'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">hSocket</span> <span class="o">==</span> <span class="n">INVALID_SOCKET</span><span class="p">)</span></div><div class='line' id='LC708'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC709'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">WSAGetLastError</span><span class="p">()</span> <span class="o">!=</span> <span class="n">WSAEWOULDBLOCK</span><span class="p">)</span></div><div class='line' id='LC710'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket error accept failed: %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">WSAGetLastError</span><span class="p">());</span></div><div class='line' id='LC711'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC712'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span> <span class="k">if</span> <span class="p">(</span><span class="n">nInbound</span> <span class="o">&gt;=</span> <span class="n">GetArg</span><span class="p">(</span><span class="s">&quot;-maxconnections&quot;</span><span class="p">,</span> <span class="mi">125</span><span class="p">)</span> <span class="o">-</span> <span class="n">MAX_OUTBOUND_CONNECTIONS</span><span class="p">)</span></div><div class='line' id='LC713'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC714'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC715'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC716'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC717'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC718'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;accepted connection %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">addr</span><span class="p">.</span><span class="n">ToStringLog</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC719'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span> <span class="o">=</span> <span class="k">new</span> <span class="n">CNode</span><span class="p">(</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">addr</span><span class="p">,</span> <span class="kc">true</span><span class="p">);</span></div><div class='line' id='LC720'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">AddRef</span><span class="p">();</span></div><div class='line' id='LC721'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC722'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vNodes</span><span class="p">.</span><span class="n">push_back</span><span class="p">(</span><span class="n">pnode</span><span class="p">);</span></div><div class='line' id='LC723'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC724'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC725'><br/></div><div class='line' id='LC726'><br/></div><div class='line' id='LC727'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC728'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Service each socket</span></div><div class='line' id='LC729'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC730'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vector</span><span class="o">&lt;</span><span class="n">CNode</span><span class="o">*&gt;</span> <span class="n">vNodesCopy</span><span class="p">;</span></div><div class='line' id='LC731'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC732'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC733'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vNodesCopy</span> <span class="o">=</span> <span class="n">vNodes</span><span class="p">;</span></div><div class='line' id='LC734'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodesCopy</span><span class="p">)</span></div><div class='line' id='LC735'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">AddRef</span><span class="p">();</span></div><div class='line' id='LC736'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC737'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodesCopy</span><span class="p">)</span></div><div class='line' id='LC738'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC739'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC740'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC741'><br/></div><div class='line' id='LC742'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC743'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Receive</span></div><div class='line' id='LC744'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC745'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span> <span class="o">==</span> <span class="n">INVALID_SOCKET</span><span class="p">)</span></div><div class='line' id='LC746'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC747'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">FD_ISSET</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetRecv</span><span class="p">)</span> <span class="o">||</span> <span class="n">FD_ISSET</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetError</span><span class="p">))</span></div><div class='line' id='LC748'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC749'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">TRY_CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_vRecv</span><span class="p">)</span></div><div class='line' id='LC750'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC751'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CDataStream</span><span class="o">&amp;</span> <span class="n">vRecv</span> <span class="o">=</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">vRecv</span><span class="p">;</span></div><div class='line' id='LC752'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">nPos</span> <span class="o">=</span> <span class="n">vRecv</span><span class="p">.</span><span class="n">size</span><span class="p">();</span></div><div class='line' id='LC753'><br/></div><div class='line' id='LC754'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nPos</span> <span class="o">&gt;</span> <span class="mi">1000</span><span class="o">*</span><span class="n">GetArg</span><span class="p">(</span><span class="s">&quot;-maxreceivebuffer&quot;</span><span class="p">,</span> <span class="mi">10</span><span class="o">*</span><span class="mi">1000</span><span class="p">))</span> <span class="p">{</span></div><div class='line' id='LC755'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fDisconnect</span><span class="p">)</span></div><div class='line' id='LC756'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket recv flood control disconnect (%d bytes)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">vRecv</span><span class="p">.</span><span class="n">size</span><span class="p">());</span></div><div class='line' id='LC757'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">CloseSocketDisconnect</span><span class="p">();</span></div><div class='line' id='LC758'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC759'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span> <span class="p">{</span></div><div class='line' id='LC760'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// typical socket buffer is 8K-64K</span></div><div class='line' id='LC761'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span> <span class="n">pchBuf</span><span class="p">[</span><span class="mh">0x10000</span><span class="p">];</span></div><div class='line' id='LC762'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nBytes</span> <span class="o">=</span> <span class="n">recv</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">,</span> <span class="n">pchBuf</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">pchBuf</span><span class="p">),</span> <span class="n">MSG_DONTWAIT</span><span class="p">);</span></div><div class='line' id='LC763'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nBytes</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC764'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC765'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vRecv</span><span class="p">.</span><span class="n">resize</span><span class="p">(</span><span class="n">nPos</span> <span class="o">+</span> <span class="n">nBytes</span><span class="p">);</span></div><div class='line' id='LC766'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">memcpy</span><span class="p">(</span><span class="o">&amp;</span><span class="n">vRecv</span><span class="p">[</span><span class="n">nPos</span><span class="p">],</span> <span class="n">pchBuf</span><span class="p">,</span> <span class="n">nBytes</span><span class="p">);</span></div><div class='line' id='LC767'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastRecv</span> <span class="o">=</span> <span class="n">GetTime</span><span class="p">();</span></div><div class='line' id='LC768'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC769'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span> <span class="k">if</span> <span class="p">(</span><span class="n">nBytes</span> <span class="o">==</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC770'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC771'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// socket closed gracefully</span></div><div class='line' id='LC772'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fDisconnect</span><span class="p">)</span></div><div class='line' id='LC773'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket closed</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC774'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">CloseSocketDisconnect</span><span class="p">();</span></div><div class='line' id='LC775'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC776'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span> <span class="k">if</span> <span class="p">(</span><span class="n">nBytes</span> <span class="o">&lt;</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC777'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC778'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// error</span></div><div class='line' id='LC779'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nErr</span> <span class="o">=</span> <span class="n">WSAGetLastError</span><span class="p">();</span></div><div class='line' id='LC780'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nErr</span> <span class="o">!=</span> <span class="n">WSAEWOULDBLOCK</span> <span class="o">&amp;&amp;</span> <span class="n">nErr</span> <span class="o">!=</span> <span class="n">WSAEMSGSIZE</span> <span class="o">&amp;&amp;</span> <span class="n">nErr</span> <span class="o">!=</span> <span class="n">WSAEINTR</span> <span class="o">&amp;&amp;</span> <span class="n">nErr</span> <span class="o">!=</span> <span class="n">WSAEINPROGRESS</span><span class="p">)</span></div><div class='line' id='LC781'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC782'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fDisconnect</span><span class="p">)</span></div><div class='line' id='LC783'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket recv error %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">nErr</span><span class="p">);</span></div><div class='line' id='LC784'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">CloseSocketDisconnect</span><span class="p">();</span></div><div class='line' id='LC785'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC786'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC787'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC788'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC789'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC790'><br/></div><div class='line' id='LC791'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC792'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Send</span></div><div class='line' id='LC793'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC794'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span> <span class="o">==</span> <span class="n">INVALID_SOCKET</span><span class="p">)</span></div><div class='line' id='LC795'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC796'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">FD_ISSET</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">fdsetSend</span><span class="p">))</span></div><div class='line' id='LC797'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC798'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">TRY_CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_vSend</span><span class="p">)</span></div><div class='line' id='LC799'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC800'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CDataStream</span><span class="o">&amp;</span> <span class="n">vSend</span> <span class="o">=</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">vSend</span><span class="p">;</span></div><div class='line' id='LC801'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">vSend</span><span class="p">.</span><span class="n">empty</span><span class="p">())</span></div><div class='line' id='LC802'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC803'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nBytes</span> <span class="o">=</span> <span class="n">send</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">vSend</span><span class="p">[</span><span class="mi">0</span><span class="p">],</span> <span class="n">vSend</span><span class="p">.</span><span class="n">size</span><span class="p">(),</span> <span class="n">MSG_NOSIGNAL</span> <span class="o">|</span> <span class="n">MSG_DONTWAIT</span><span class="p">);</span></div><div class='line' id='LC804'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nBytes</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC805'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC806'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vSend</span><span class="p">.</span><span class="n">erase</span><span class="p">(</span><span class="n">vSend</span><span class="p">.</span><span class="n">begin</span><span class="p">(),</span> <span class="n">vSend</span><span class="p">.</span><span class="n">begin</span><span class="p">()</span> <span class="o">+</span> <span class="n">nBytes</span><span class="p">);</span></div><div class='line' id='LC807'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastSend</span> <span class="o">=</span> <span class="n">GetTime</span><span class="p">();</span></div><div class='line' id='LC808'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC809'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span> <span class="k">if</span> <span class="p">(</span><span class="n">nBytes</span> <span class="o">&lt;</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC810'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC811'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// error</span></div><div class='line' id='LC812'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nErr</span> <span class="o">=</span> <span class="n">WSAGetLastError</span><span class="p">();</span></div><div class='line' id='LC813'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nErr</span> <span class="o">!=</span> <span class="n">WSAEWOULDBLOCK</span> <span class="o">&amp;&amp;</span> <span class="n">nErr</span> <span class="o">!=</span> <span class="n">WSAEMSGSIZE</span> <span class="o">&amp;&amp;</span> <span class="n">nErr</span> <span class="o">!=</span> <span class="n">WSAEINTR</span> <span class="o">&amp;&amp;</span> <span class="n">nErr</span> <span class="o">!=</span> <span class="n">WSAEINPROGRESS</span><span class="p">)</span></div><div class='line' id='LC814'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC815'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket send error %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">nErr</span><span class="p">);</span></div><div class='line' id='LC816'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">CloseSocketDisconnect</span><span class="p">();</span></div><div class='line' id='LC817'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC818'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC819'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vSend</span><span class="p">.</span><span class="n">size</span><span class="p">()</span> <span class="o">&gt;</span> <span class="mi">1000</span><span class="o">*</span><span class="n">GetArg</span><span class="p">(</span><span class="s">&quot;-maxsendbuffer&quot;</span><span class="p">,</span> <span class="mi">10</span><span class="o">*</span><span class="mi">1000</span><span class="p">))</span> <span class="p">{</span></div><div class='line' id='LC820'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fDisconnect</span><span class="p">)</span></div><div class='line' id='LC821'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket send flood control disconnect (%d bytes)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">vSend</span><span class="p">.</span><span class="n">size</span><span class="p">());</span></div><div class='line' id='LC822'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">CloseSocketDisconnect</span><span class="p">();</span></div><div class='line' id='LC823'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC824'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC825'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC826'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC827'><br/></div><div class='line' id='LC828'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC829'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Inactivity checking</span></div><div class='line' id='LC830'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC831'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">vSend</span><span class="p">.</span><span class="n">empty</span><span class="p">())</span></div><div class='line' id='LC832'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastSendEmpty</span> <span class="o">=</span> <span class="n">GetTime</span><span class="p">();</span></div><div class='line' id='LC833'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nTimeConnected</span> <span class="o">&gt;</span> <span class="mi">60</span><span class="p">)</span></div><div class='line' id='LC834'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC835'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastRecv</span> <span class="o">==</span> <span class="mi">0</span> <span class="o">||</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastSend</span> <span class="o">==</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC836'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC837'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket no message in first 60 seconds, %d %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastRecv</span> <span class="o">!=</span> <span class="mi">0</span><span class="p">,</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastSend</span> <span class="o">!=</span> <span class="mi">0</span><span class="p">);</span></div><div class='line' id='LC838'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fDisconnect</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC839'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC840'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span> <span class="k">if</span> <span class="p">(</span><span class="n">GetTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastSend</span> <span class="o">&gt;</span> <span class="mi">90</span><span class="o">*</span><span class="mi">60</span> <span class="o">&amp;&amp;</span> <span class="n">GetTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastSendEmpty</span> <span class="o">&gt;</span> <span class="mi">90</span><span class="o">*</span><span class="mi">60</span><span class="p">)</span></div><div class='line' id='LC841'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC842'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket not sending</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC843'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fDisconnect</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC844'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC845'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span> <span class="k">if</span> <span class="p">(</span><span class="n">GetTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">pnode</span><span class="o">-&gt;</span><span class="n">nLastRecv</span> <span class="o">&gt;</span> <span class="mi">90</span><span class="o">*</span><span class="mi">60</span><span class="p">)</span></div><div class='line' id='LC846'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC847'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;socket inactivity timeout</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC848'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fDisconnect</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC849'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC850'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC851'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC852'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC853'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC854'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodesCopy</span><span class="p">)</span></div><div class='line' id='LC855'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">Release</span><span class="p">();</span></div><div class='line' id='LC856'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC857'><br/></div><div class='line' id='LC858'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">10</span><span class="p">);</span></div><div class='line' id='LC859'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC860'><span class="p">}</span></div><div class='line' id='LC861'><br/></div><div class='line' id='LC862'><br/></div><div class='line' id='LC863'><br/></div><div class='line' id='LC864'><br/></div><div class='line' id='LC865'><br/></div><div class='line' id='LC866'><br/></div><div class='line' id='LC867'><br/></div><div class='line' id='LC868'><br/></div><div class='line' id='LC869'><br/></div><div class='line' id='LC870'><span class="cp">#ifdef USE_UPNP</span></div><div class='line' id='LC871'><span class="kt">void</span> <span class="n">ThreadMapPort</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC872'><span class="p">{</span></div><div class='line' id='LC873'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">IMPLEMENT_RANDOMIZE_STACK</span><span class="p">(</span><span class="n">ThreadMapPort</span><span class="p">(</span><span class="n">parg</span><span class="p">));</span></div><div class='line' id='LC874'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">try</span></div><div class='line' id='LC875'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC876'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">5</span><span class="p">]</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC877'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ThreadMapPort2</span><span class="p">(</span><span class="n">parg</span><span class="p">);</span></div><div class='line' id='LC878'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">5</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC879'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC880'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">catch</span> <span class="p">(</span><span class="n">std</span><span class="o">::</span><span class="n">exception</span><span class="o">&amp;</span> <span class="n">e</span><span class="p">)</span> <span class="p">{</span></div><div class='line' id='LC881'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">5</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC882'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintException</span><span class="p">(</span><span class="o">&amp;</span><span class="n">e</span><span class="p">,</span> <span class="s">&quot;ThreadMapPort()&quot;</span><span class="p">);</span></div><div class='line' id='LC883'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span> <span class="k">catch</span> <span class="p">(...)</span> <span class="p">{</span></div><div class='line' id='LC884'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">5</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC885'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintException</span><span class="p">(</span><span class="nb">NULL</span><span class="p">,</span> <span class="s">&quot;ThreadMapPort()&quot;</span><span class="p">);</span></div><div class='line' id='LC886'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC887'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadMapPort exiting</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC888'><span class="p">}</span></div><div class='line' id='LC889'><br/></div><div class='line' id='LC890'><span class="kt">void</span> <span class="n">ThreadMapPort2</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC891'><span class="p">{</span></div><div class='line' id='LC892'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadMapPort started</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC893'><br/></div><div class='line' id='LC894'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span> <span class="n">port</span><span class="p">[</span><span class="mi">6</span><span class="p">];</span></div><div class='line' id='LC895'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">sprintf</span><span class="p">(</span><span class="n">port</span><span class="p">,</span> <span class="s">&quot;%d&quot;</span><span class="p">,</span> <span class="n">GetDefaultPort</span><span class="p">());</span></div><div class='line' id='LC896'><br/></div><div class='line' id='LC897'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">const</span> <span class="kt">char</span> <span class="o">*</span> <span class="n">rootdescurl</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC898'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">const</span> <span class="kt">char</span> <span class="o">*</span> <span class="n">multicastif</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC899'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">const</span> <span class="kt">char</span> <span class="o">*</span> <span class="n">minissdpdpath</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC900'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">UPNPDev</span> <span class="o">*</span> <span class="n">devlist</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC901'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span> <span class="n">lanaddr</span><span class="p">[</span><span class="mi">64</span><span class="p">];</span></div><div class='line' id='LC902'><br/></div><div class='line' id='LC903'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">devlist</span> <span class="o">=</span> <span class="n">upnpDiscover</span><span class="p">(</span><span class="mi">2000</span><span class="p">,</span> <span class="n">multicastif</span><span class="p">,</span> <span class="n">minissdpdpath</span><span class="p">,</span> <span class="mi">0</span><span class="p">);</span></div><div class='line' id='LC904'><br/></div><div class='line' id='LC905'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">UPNPUrls</span> <span class="n">urls</span><span class="p">;</span></div><div class='line' id='LC906'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">IGDdatas</span> <span class="n">data</span><span class="p">;</span></div><div class='line' id='LC907'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">r</span><span class="p">;</span></div><div class='line' id='LC908'><br/></div><div class='line' id='LC909'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">UPNP_GetValidIGD</span><span class="p">(</span><span class="n">devlist</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">urls</span><span class="p">,</span> <span class="o">&amp;</span><span class="n">data</span><span class="p">,</span> <span class="n">lanaddr</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">lanaddr</span><span class="p">))</span> <span class="o">==</span> <span class="mi">1</span><span class="p">)</span></div><div class='line' id='LC910'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC911'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span> <span class="n">intClient</span><span class="p">[</span><span class="mi">16</span><span class="p">];</span></div><div class='line' id='LC912'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span> <span class="n">intPort</span><span class="p">[</span><span class="mi">6</span><span class="p">];</span></div><div class='line' id='LC913'><br/></div><div class='line' id='LC914'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">r</span> <span class="o">=</span> <span class="n">UPNP_AddPortMapping</span><span class="p">(</span><span class="n">urls</span><span class="p">.</span><span class="n">controlURL</span><span class="p">,</span> <span class="n">data</span><span class="p">.</span><span class="n">first</span><span class="p">.</span><span class="n">servicetype</span><span class="p">,</span></div><div class='line' id='LC915'>	                        <span class="n">port</span><span class="p">,</span> <span class="n">port</span><span class="p">,</span> <span class="n">lanaddr</span><span class="p">,</span> <span class="mi">0</span><span class="p">,</span> <span class="s">&quot;TCP&quot;</span><span class="p">,</span> <span class="mi">0</span><span class="p">);</span></div><div class='line' id='LC916'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span><span class="p">(</span><span class="n">r</span><span class="o">!=</span><span class="n">UPNPCOMMAND_SUCCESS</span><span class="p">)</span></div><div class='line' id='LC917'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;AddPortMapping(%s, %s, %s) failed with code %d (%s)</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span></div><div class='line' id='LC918'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">port</span><span class="p">,</span> <span class="n">port</span><span class="p">,</span> <span class="n">lanaddr</span><span class="p">,</span> <span class="n">r</span><span class="p">,</span> <span class="n">strupnperror</span><span class="p">(</span><span class="n">r</span><span class="p">));</span></div><div class='line' id='LC919'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC920'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;UPnP Port Mapping successful.</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC921'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">loop</span> <span class="p">{</span></div><div class='line' id='LC922'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC923'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC924'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">r</span> <span class="o">=</span> <span class="n">UPNP_DeletePortMapping</span><span class="p">(</span><span class="n">urls</span><span class="p">.</span><span class="n">controlURL</span><span class="p">,</span> <span class="n">data</span><span class="p">.</span><span class="n">first</span><span class="p">.</span><span class="n">servicetype</span><span class="p">,</span> <span class="n">port</span><span class="p">,</span> <span class="s">&quot;TCP&quot;</span><span class="p">,</span> <span class="mi">0</span><span class="p">);</span></div><div class='line' id='LC925'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;UPNP_DeletePortMapping() returned : %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">r</span><span class="p">);</span></div><div class='line' id='LC926'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">freeUPNPDevlist</span><span class="p">(</span><span class="n">devlist</span><span class="p">);</span> <span class="n">devlist</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC927'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FreeUPNPUrls</span><span class="p">(</span><span class="o">&amp;</span><span class="n">urls</span><span class="p">);</span></div><div class='line' id='LC928'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC929'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC930'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">2000</span><span class="p">);</span></div><div class='line' id='LC931'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC932'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span> <span class="k">else</span> <span class="p">{</span></div><div class='line' id='LC933'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;No valid UPnP IGDs found</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC934'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">freeUPNPDevlist</span><span class="p">(</span><span class="n">devlist</span><span class="p">);</span> <span class="n">devlist</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC935'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">FreeUPNPUrls</span><span class="p">(</span><span class="o">&amp;</span><span class="n">urls</span><span class="p">);</span></div><div class='line' id='LC936'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">loop</span> <span class="p">{</span></div><div class='line' id='LC937'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC938'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC939'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">2000</span><span class="p">);</span></div><div class='line' id='LC940'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC941'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC942'><span class="p">}</span></div><div class='line' id='LC943'><span class="cp">#endif</span></div><div class='line' id='LC944'><br/></div><div class='line' id='LC945'><br/></div><div class='line' id='LC946'><br/></div><div class='line' id='LC947'><br/></div><div class='line' id='LC948'><br/></div><div class='line' id='LC949'><br/></div><div class='line' id='LC950'><br/></div><div class='line' id='LC951'><br/></div><div class='line' id='LC952'><br/></div><div class='line' id='LC953'><br/></div><div class='line' id='LC954'><br/></div><div class='line' id='LC955'><br/></div><div class='line' id='LC956'><span class="kt">unsigned</span> <span class="kt">int</span> <span class="n">pnSeed</span><span class="p">[]</span> <span class="o">=</span></div><div class='line' id='LC957'><span class="p">{</span></div><div class='line' id='LC958'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x1ddb1032</span><span class="p">,</span> <span class="mh">0x6242ce40</span><span class="p">,</span> <span class="mh">0x52d6a445</span><span class="p">,</span> <span class="mh">0x2dd7a445</span><span class="p">,</span> <span class="mh">0x8a53cd47</span><span class="p">,</span> <span class="mh">0x73263750</span><span class="p">,</span> <span class="mh">0xda23c257</span><span class="p">,</span> <span class="mh">0xecd4ed57</span><span class="p">,</span></div><div class='line' id='LC959'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x0a40ec59</span><span class="p">,</span> <span class="mh">0x75dce160</span><span class="p">,</span> <span class="mh">0x7df76791</span><span class="p">,</span> <span class="mh">0x89370bad</span><span class="p">,</span> <span class="mh">0xa4f214ad</span><span class="p">,</span> <span class="mh">0x767700ae</span><span class="p">,</span> <span class="mh">0x638b0418</span><span class="p">,</span> <span class="mh">0x868a1018</span><span class="p">,</span></div><div class='line' id='LC960'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xcd9f332e</span><span class="p">,</span> <span class="mh">0x0129653e</span><span class="p">,</span> <span class="mh">0xcc92dc3e</span><span class="p">,</span> <span class="mh">0x96671640</span><span class="p">,</span> <span class="mh">0x56487e40</span><span class="p">,</span> <span class="mh">0x5b66f440</span><span class="p">,</span> <span class="mh">0xb1d01f41</span><span class="p">,</span> <span class="mh">0xf1dc6041</span><span class="p">,</span></div><div class='line' id='LC961'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xc1d12b42</span><span class="p">,</span> <span class="mh">0x86ba1243</span><span class="p">,</span> <span class="mh">0x6be4df43</span><span class="p">,</span> <span class="mh">0x6d4cef43</span><span class="p">,</span> <span class="mh">0xd18e0644</span><span class="p">,</span> <span class="mh">0x1ab0b344</span><span class="p">,</span> <span class="mh">0x6584a345</span><span class="p">,</span> <span class="mh">0xe7c1a445</span><span class="p">,</span></div><div class='line' id='LC962'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x58cea445</span><span class="p">,</span> <span class="mh">0xc5daa445</span><span class="p">,</span> <span class="mh">0x21dda445</span><span class="p">,</span> <span class="mh">0x3d3b5346</span><span class="p">,</span> <span class="mh">0x13e55347</span><span class="p">,</span> <span class="mh">0x1080d24a</span><span class="p">,</span> <span class="mh">0x8e611e4b</span><span class="p">,</span> <span class="mh">0x81518e4b</span><span class="p">,</span></div><div class='line' id='LC963'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x6c839e4b</span><span class="p">,</span> <span class="mh">0xe2ad0a4c</span><span class="p">,</span> <span class="mh">0xfbbc0a4c</span><span class="p">,</span> <span class="mh">0x7f5b6e4c</span><span class="p">,</span> <span class="mh">0x7244224e</span><span class="p">,</span> <span class="mh">0x1300554e</span><span class="p">,</span> <span class="mh">0x20690652</span><span class="p">,</span> <span class="mh">0x5a48b652</span><span class="p">,</span></div><div class='line' id='LC964'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x75c5c752</span><span class="p">,</span> <span class="mh">0x4335cc54</span><span class="p">,</span> <span class="mh">0x340fd154</span><span class="p">,</span> <span class="mh">0x87c07455</span><span class="p">,</span> <span class="mh">0x087b2b56</span><span class="p">,</span> <span class="mh">0x8a133a57</span><span class="p">,</span> <span class="mh">0xac23c257</span><span class="p">,</span> <span class="mh">0x70374959</span><span class="p">,</span></div><div class='line' id='LC965'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xfb63d45b</span><span class="p">,</span> <span class="mh">0xb9a1685c</span><span class="p">,</span> <span class="mh">0x180d765c</span><span class="p">,</span> <span class="mh">0x674f645d</span><span class="p">,</span> <span class="mh">0x04d3495e</span><span class="p">,</span> <span class="mh">0x1de44b5e</span><span class="p">,</span> <span class="mh">0x4ee8a362</span><span class="p">,</span> <span class="mh">0x0ded1b63</span><span class="p">,</span></div><div class='line' id='LC966'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xc1b04b6d</span><span class="p">,</span> <span class="mh">0x8d921581</span><span class="p">,</span> <span class="mh">0x97b7ea82</span><span class="p">,</span> <span class="mh">0x1cf83a8e</span><span class="p">,</span> <span class="mh">0x91490bad</span><span class="p">,</span> <span class="mh">0x09dc75ae</span><span class="p">,</span> <span class="mh">0x9a6d79ae</span><span class="p">,</span> <span class="mh">0xa26d79ae</span><span class="p">,</span></div><div class='line' id='LC967'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x0fd08fae</span><span class="p">,</span> <span class="mh">0x0f3e3fb2</span><span class="p">,</span> <span class="mh">0x4f944fb2</span><span class="p">,</span> <span class="mh">0xcca448b8</span><span class="p">,</span> <span class="mh">0x3ecd6ab8</span><span class="p">,</span> <span class="mh">0xa9d5a5bc</span><span class="p">,</span> <span class="mh">0x8d0119c1</span><span class="p">,</span> <span class="mh">0x045997d5</span><span class="p">,</span></div><div class='line' id='LC968'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xca019dd9</span><span class="p">,</span> <span class="mh">0x0d526c4d</span><span class="p">,</span> <span class="mh">0xabf1ba44</span><span class="p">,</span> <span class="mh">0x66b1ab55</span><span class="p">,</span> <span class="mh">0x1165f462</span><span class="p">,</span> <span class="mh">0x3ed7cbad</span><span class="p">,</span> <span class="mh">0xa38fae6e</span><span class="p">,</span> <span class="mh">0x3bd2cbad</span><span class="p">,</span></div><div class='line' id='LC969'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xd36f0547</span><span class="p">,</span> <span class="mh">0x20df7840</span><span class="p">,</span> <span class="mh">0x7a337742</span><span class="p">,</span> <span class="mh">0x549f8e4b</span><span class="p">,</span> <span class="mh">0x9062365c</span><span class="p">,</span> <span class="mh">0xd399f562</span><span class="p">,</span> <span class="mh">0x2b5274a1</span><span class="p">,</span> <span class="mh">0x8edfa153</span><span class="p">,</span></div><div class='line' id='LC970'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x3bffb347</span><span class="p">,</span> <span class="mh">0x7074bf58</span><span class="p">,</span> <span class="mh">0xb74fcbad</span><span class="p">,</span> <span class="mh">0x5b5a795b</span><span class="p">,</span> <span class="mh">0x02fa29ce</span><span class="p">,</span> <span class="mh">0x5a6738d4</span><span class="p">,</span> <span class="mh">0xe8a1d23e</span><span class="p">,</span> <span class="mh">0xef98c445</span><span class="p">,</span></div><div class='line' id='LC971'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x4b0f494c</span><span class="p">,</span> <span class="mh">0xa2bc1e56</span><span class="p">,</span> <span class="mh">0x7694ad63</span><span class="p">,</span> <span class="mh">0xa4a800c3</span><span class="p">,</span> <span class="mh">0x05fda6cd</span><span class="p">,</span> <span class="mh">0x9f22175e</span><span class="p">,</span> <span class="mh">0x364a795b</span><span class="p">,</span> <span class="mh">0x536285d5</span><span class="p">,</span></div><div class='line' id='LC972'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xac44c9d4</span><span class="p">,</span> <span class="mh">0x0b06254d</span><span class="p">,</span> <span class="mh">0x150c2fd4</span><span class="p">,</span> <span class="mh">0x32a50dcc</span><span class="p">,</span> <span class="mh">0xfd79ce48</span><span class="p">,</span> <span class="mh">0xf15cfa53</span><span class="p">,</span> <span class="mh">0x66c01e60</span><span class="p">,</span> <span class="mh">0x6bc26661</span><span class="p">,</span></div><div class='line' id='LC973'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xc03b47ae</span><span class="p">,</span> <span class="mh">0x4dda1b81</span><span class="p">,</span> <span class="mh">0x3285a4c1</span><span class="p">,</span> <span class="mh">0x883ca96d</span><span class="p">,</span> <span class="mh">0x35d60a4c</span><span class="p">,</span> <span class="mh">0xdae09744</span><span class="p">,</span> <span class="mh">0x2e314d61</span><span class="p">,</span> <span class="mh">0x84e247cf</span><span class="p">,</span></div><div class='line' id='LC974'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x6c814552</span><span class="p">,</span> <span class="mh">0x3a1cc658</span><span class="p">,</span> <span class="mh">0x98d8f382</span><span class="p">,</span> <span class="mh">0xe584cb5b</span><span class="p">,</span> <span class="mh">0x15e86057</span><span class="p">,</span> <span class="mh">0x7b01504e</span><span class="p">,</span> <span class="mh">0xd852dd48</span><span class="p">,</span> <span class="mh">0x56382f56</span><span class="p">,</span></div><div class='line' id='LC975'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x0a5df454</span><span class="p">,</span> <span class="mh">0xa0d18d18</span><span class="p">,</span> <span class="mh">0x2e89b148</span><span class="p">,</span> <span class="mh">0xa79c114c</span><span class="p">,</span> <span class="mh">0xcbdcd054</span><span class="p">,</span> <span class="mh">0x5523bc43</span><span class="p">,</span> <span class="mh">0xa9832640</span><span class="p">,</span> <span class="mh">0x8a066144</span><span class="p">,</span></div><div class='line' id='LC976'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x3894c3bc</span><span class="p">,</span> <span class="mh">0xab76bf58</span><span class="p">,</span> <span class="mh">0x6a018ac1</span><span class="p">,</span> <span class="mh">0xfebf4f43</span><span class="p">,</span> <span class="mh">0x2f26c658</span><span class="p">,</span> <span class="mh">0x31102f4e</span><span class="p">,</span> <span class="mh">0x85e929d5</span><span class="p">,</span> <span class="mh">0x2a1c175e</span><span class="p">,</span></div><div class='line' id='LC977'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xfc6c2cd1</span><span class="p">,</span> <span class="mh">0x27b04b6d</span><span class="p">,</span> <span class="mh">0xdf024650</span><span class="p">,</span> <span class="mh">0x161748b8</span><span class="p">,</span> <span class="mh">0x28be6580</span><span class="p">,</span> <span class="mh">0x57be6580</span><span class="p">,</span> <span class="mh">0x1cee677a</span><span class="p">,</span> <span class="mh">0xaa6bb742</span><span class="p">,</span></div><div class='line' id='LC978'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x9a53964b</span><span class="p">,</span> <span class="mh">0x0a5a2d4d</span><span class="p">,</span> <span class="mh">0x2434c658</span><span class="p">,</span> <span class="mh">0x9a494f57</span><span class="p">,</span> <span class="mh">0x1ebb0e48</span><span class="p">,</span> <span class="mh">0xf610b85d</span><span class="p">,</span> <span class="mh">0x077ecf44</span><span class="p">,</span> <span class="mh">0x085128bc</span><span class="p">,</span></div><div class='line' id='LC979'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x5ba17a18</span><span class="p">,</span> <span class="mh">0x27ca1b42</span><span class="p">,</span> <span class="mh">0xf8a00b56</span><span class="p">,</span> <span class="mh">0xfcd4c257</span><span class="p">,</span> <span class="mh">0xcf2fc15e</span><span class="p">,</span> <span class="mh">0xd897e052</span><span class="p">,</span> <span class="mh">0x4cada04f</span><span class="p">,</span> <span class="mh">0x2f35f6d5</span><span class="p">,</span></div><div class='line' id='LC980'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x382ce8c9</span><span class="p">,</span> <span class="mh">0xe523984b</span><span class="p">,</span> <span class="mh">0x3f946846</span><span class="p">,</span> <span class="mh">0x60c8be43</span><span class="p">,</span> <span class="mh">0x41da6257</span><span class="p">,</span> <span class="mh">0xde0be142</span><span class="p">,</span> <span class="mh">0xae8a544b</span><span class="p">,</span> <span class="mh">0xeff0c254</span><span class="p">,</span></div><div class='line' id='LC981'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x1e0f795b</span><span class="p">,</span> <span class="mh">0xaeb28890</span><span class="p">,</span> <span class="mh">0xca16acd9</span><span class="p">,</span> <span class="mh">0x1e47ddd8</span><span class="p">,</span> <span class="mh">0x8c8c4829</span><span class="p">,</span> <span class="mh">0xd27dc747</span><span class="p">,</span> <span class="mh">0xd53b1663</span><span class="p">,</span> <span class="mh">0x4096b163</span><span class="p">,</span></div><div class='line' id='LC982'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x9c8dd958</span><span class="p">,</span> <span class="mh">0xcb12f860</span><span class="p">,</span> <span class="mh">0x9e79305c</span><span class="p">,</span> <span class="mh">0x40c1a445</span><span class="p">,</span> <span class="mh">0x4a90c2bc</span><span class="p">,</span> <span class="mh">0x2c3a464d</span><span class="p">,</span> <span class="mh">0x2727f23c</span><span class="p">,</span> <span class="mh">0x30b04b6d</span><span class="p">,</span></div><div class='line' id='LC983'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x59024cb8</span><span class="p">,</span> <span class="mh">0xa091e6ad</span><span class="p">,</span> <span class="mh">0x31b04b6d</span><span class="p">,</span> <span class="mh">0xc29d46a6</span><span class="p">,</span> <span class="mh">0x63934fb2</span><span class="p">,</span> <span class="mh">0xd9224dbe</span><span class="p">,</span> <span class="mh">0x9f5910d8</span><span class="p">,</span> <span class="mh">0x7f530a6b</span><span class="p">,</span></div><div class='line' id='LC984'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x752e9c95</span><span class="p">,</span> <span class="mh">0x65453548</span><span class="p">,</span> <span class="mh">0xa484be46</span><span class="p">,</span> <span class="mh">0xce5a1b59</span><span class="p">,</span> <span class="mh">0x710e0718</span><span class="p">,</span> <span class="mh">0x46a13d18</span><span class="p">,</span> <span class="mh">0xdaaf5318</span><span class="p">,</span> <span class="mh">0xc4a8ff53</span><span class="p">,</span></div><div class='line' id='LC985'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x87abaa52</span><span class="p">,</span> <span class="mh">0xb764cf51</span><span class="p">,</span> <span class="mh">0xb2025d4a</span><span class="p">,</span> <span class="mh">0x6d351e41</span><span class="p">,</span> <span class="mh">0xc035c33e</span><span class="p">,</span> <span class="mh">0xa432c162</span><span class="p">,</span> <span class="mh">0x61ef34ae</span><span class="p">,</span> <span class="mh">0xd16fddbc</span><span class="p">,</span></div><div class='line' id='LC986'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x0870e8c1</span><span class="p">,</span> <span class="mh">0x3070e8c1</span><span class="p">,</span> <span class="mh">0x9c71e8c1</span><span class="p">,</span> <span class="mh">0xa4992363</span><span class="p">,</span> <span class="mh">0x85a1f663</span><span class="p">,</span> <span class="mh">0x4184e559</span><span class="p">,</span> <span class="mh">0x18d96ed8</span><span class="p">,</span> <span class="mh">0x17b8dbd5</span><span class="p">,</span></div><div class='line' id='LC987'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x60e7cd18</span><span class="p">,</span> <span class="mh">0xe5ee104c</span><span class="p">,</span> <span class="mh">0xab17ac62</span><span class="p">,</span> <span class="mh">0x1e786e1b</span><span class="p">,</span> <span class="mh">0x5d23b762</span><span class="p">,</span> <span class="mh">0xf2388fae</span><span class="p">,</span> <span class="mh">0x88270360</span><span class="p">,</span> <span class="mh">0x9e5b3d80</span><span class="p">,</span></div><div class='line' id='LC988'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x7da518b2</span><span class="p">,</span> <span class="mh">0xb5613b45</span><span class="p">,</span> <span class="mh">0x1ad41f3e</span><span class="p">,</span> <span class="mh">0xd550854a</span><span class="p">,</span> <span class="mh">0x8617e9a9</span><span class="p">,</span> <span class="mh">0x925b229c</span><span class="p">,</span> <span class="mh">0xf2e92542</span><span class="p">,</span> <span class="mh">0x47af0544</span><span class="p">,</span></div><div class='line' id='LC989'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x73b5a843</span><span class="p">,</span> <span class="mh">0xb9b7a0ad</span><span class="p">,</span> <span class="mh">0x03a748d0</span><span class="p">,</span> <span class="mh">0x0a6ff862</span><span class="p">,</span> <span class="mh">0x6694df62</span><span class="p">,</span> <span class="mh">0x3bfac948</span><span class="p">,</span> <span class="mh">0x8e098f4f</span><span class="p">,</span> <span class="mh">0x746916c3</span><span class="p">,</span></div><div class='line' id='LC990'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x02f38e4f</span><span class="p">,</span> <span class="mh">0x40bb1243</span><span class="p">,</span> <span class="mh">0x6a54d162</span><span class="p">,</span> <span class="mh">0x6008414b</span><span class="p">,</span> <span class="mh">0xa513794c</span><span class="p">,</span> <span class="mh">0x514aa343</span><span class="p">,</span> <span class="mh">0x63781747</span><span class="p">,</span> <span class="mh">0xdbb6795b</span><span class="p">,</span></div><div class='line' id='LC991'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xed065058</span><span class="p">,</span> <span class="mh">0x42d24b46</span><span class="p">,</span> <span class="mh">0x1518794c</span><span class="p">,</span> <span class="mh">0x9b271681</span><span class="p">,</span> <span class="mh">0x73e4ffad</span><span class="p">,</span> <span class="mh">0x0654784f</span><span class="p">,</span> <span class="mh">0x438dc945</span><span class="p">,</span> <span class="mh">0x641846a6</span><span class="p">,</span></div><div class='line' id='LC992'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x2d1b0944</span><span class="p">,</span> <span class="mh">0x94b59148</span><span class="p">,</span> <span class="mh">0x8d369558</span><span class="p">,</span> <span class="mh">0xa5a97662</span><span class="p">,</span> <span class="mh">0x8b705b42</span><span class="p">,</span> <span class="mh">0xce9204ae</span><span class="p">,</span> <span class="mh">0x8d584450</span><span class="p">,</span> <span class="mh">0x2df61555</span><span class="p">,</span></div><div class='line' id='LC993'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xeebff943</span><span class="p">,</span> <span class="mh">0x2e75fb4d</span><span class="p">,</span> <span class="mh">0x3ef8fc57</span><span class="p">,</span> <span class="mh">0x9921135e</span><span class="p">,</span> <span class="mh">0x8e31042e</span><span class="p">,</span> <span class="mh">0xb5afad43</span><span class="p">,</span> <span class="mh">0x89ecedd1</span><span class="p">,</span> <span class="mh">0x9cfcc047</span><span class="p">,</span></div><div class='line' id='LC994'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x8fcd0f4c</span><span class="p">,</span> <span class="mh">0xbe49f5ad</span><span class="p">,</span> <span class="mh">0x146a8d45</span><span class="p">,</span> <span class="mh">0x98669ab8</span><span class="p">,</span> <span class="mh">0x98d9175e</span><span class="p">,</span> <span class="mh">0xd1a8e46d</span><span class="p">,</span> <span class="mh">0x839a3ab8</span><span class="p">,</span> <span class="mh">0x40a0016c</span><span class="p">,</span></div><div class='line' id='LC995'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x6d27c257</span><span class="p">,</span> <span class="mh">0x977fffad</span><span class="p">,</span> <span class="mh">0x7baa5d5d</span><span class="p">,</span> <span class="mh">0x1213be43</span><span class="p">,</span> <span class="mh">0xb167e5a9</span><span class="p">,</span> <span class="mh">0x640fe8ca</span><span class="p">,</span> <span class="mh">0xbc9ea655</span><span class="p">,</span> <span class="mh">0x0f820a4c</span><span class="p">,</span></div><div class='line' id='LC996'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0x0f097059</span><span class="p">,</span> <span class="mh">0x69ac957c</span><span class="p">,</span> <span class="mh">0x366d8453</span><span class="p">,</span> <span class="mh">0xb1ba2844</span><span class="p">,</span> <span class="mh">0x8857f081</span><span class="p">,</span> <span class="mh">0x70b5be63</span><span class="p">,</span> <span class="mh">0xc545454b</span><span class="p">,</span> <span class="mh">0xaf36ded1</span><span class="p">,</span></div><div class='line' id='LC997'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="mh">0xb5a4b052</span><span class="p">,</span> <span class="mh">0x21f062d1</span><span class="p">,</span> <span class="mh">0x72ab89b2</span><span class="p">,</span> <span class="mh">0x74a45318</span><span class="p">,</span> <span class="mh">0x8312e6bc</span><span class="p">,</span> <span class="mh">0xb916965f</span><span class="p">,</span> <span class="mh">0x8aa7c858</span><span class="p">,</span> <span class="mh">0xfe7effad</span><span class="p">,</span></div><div class='line' id='LC998'><span class="p">};</span></div><div class='line' id='LC999'><br/></div><div class='line' id='LC1000'><br/></div><div class='line' id='LC1001'><br/></div><div class='line' id='LC1002'><span class="kt">void</span> <span class="n">ThreadOpenConnections</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC1003'><span class="p">{</span></div><div class='line' id='LC1004'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">IMPLEMENT_RANDOMIZE_STACK</span><span class="p">(</span><span class="n">ThreadOpenConnections</span><span class="p">(</span><span class="n">parg</span><span class="p">));</span></div><div class='line' id='LC1005'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">try</span></div><div class='line' id='LC1006'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1007'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC1008'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ThreadOpenConnections2</span><span class="p">(</span><span class="n">parg</span><span class="p">);</span></div><div class='line' id='LC1009'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC1010'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1011'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">catch</span> <span class="p">(</span><span class="n">std</span><span class="o">::</span><span class="n">exception</span><span class="o">&amp;</span> <span class="n">e</span><span class="p">)</span> <span class="p">{</span></div><div class='line' id='LC1012'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC1013'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintException</span><span class="p">(</span><span class="o">&amp;</span><span class="n">e</span><span class="p">,</span> <span class="s">&quot;ThreadOpenConnections()&quot;</span><span class="p">);</span></div><div class='line' id='LC1014'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span> <span class="k">catch</span> <span class="p">(...)</span> <span class="p">{</span></div><div class='line' id='LC1015'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC1016'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintException</span><span class="p">(</span><span class="nb">NULL</span><span class="p">,</span> <span class="s">&quot;ThreadOpenConnections()&quot;</span><span class="p">);</span></div><div class='line' id='LC1017'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1018'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadOpenConnections exiting</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1019'><span class="p">}</span></div><div class='line' id='LC1020'><br/></div><div class='line' id='LC1021'><span class="kt">void</span> <span class="n">ThreadOpenConnections2</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC1022'><span class="p">{</span></div><div class='line' id='LC1023'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadOpenConnections started</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1024'><br/></div><div class='line' id='LC1025'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Connect to specific addresses</span></div><div class='line' id='LC1026'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;-connect&quot;</span><span class="p">))</span></div><div class='line' id='LC1027'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1028'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="n">int64</span> <span class="n">nLoop</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;;</span> <span class="n">nLoop</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC1029'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1030'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">string</span> <span class="n">strAddr</span><span class="p">,</span> <span class="n">mapMultiArgs</span><span class="p">[</span><span class="s">&quot;-connect&quot;</span><span class="p">])</span></div><div class='line' id='LC1031'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1032'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addr</span><span class="p">(</span><span class="n">strAddr</span><span class="p">,</span> <span class="n">NODE_NETWORK</span><span class="p">);</span></div><div class='line' id='LC1033'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">IsValid</span><span class="p">())</span></div><div class='line' id='LC1034'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">OpenNetworkConnection</span><span class="p">(</span><span class="n">addr</span><span class="p">);</span></div><div class='line' id='LC1035'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">i</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="n">i</span> <span class="o">&lt;</span> <span class="mi">10</span> <span class="o">&amp;&amp;</span> <span class="n">i</span> <span class="o">&lt;</span> <span class="n">nLoop</span><span class="p">;</span> <span class="n">i</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC1036'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1037'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">500</span><span class="p">);</span></div><div class='line' id='LC1038'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1039'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC1040'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1041'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1042'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1043'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1044'><br/></div><div class='line' id='LC1045'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Connect to manually added nodes first</span></div><div class='line' id='LC1046'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;-addnode&quot;</span><span class="p">))</span></div><div class='line' id='LC1047'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1048'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">string</span> <span class="n">strAddr</span><span class="p">,</span> <span class="n">mapMultiArgs</span><span class="p">[</span><span class="s">&quot;-addnode&quot;</span><span class="p">])</span></div><div class='line' id='LC1049'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1050'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addr</span><span class="p">(</span><span class="n">strAddr</span><span class="p">,</span> <span class="n">NODE_NETWORK</span><span class="p">);</span></div><div class='line' id='LC1051'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">IsValid</span><span class="p">())</span></div><div class='line' id='LC1052'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1053'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">OpenNetworkConnection</span><span class="p">(</span><span class="n">addr</span><span class="p">);</span></div><div class='line' id='LC1054'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">500</span><span class="p">);</span></div><div class='line' id='LC1055'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1056'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC1057'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1058'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1059'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1060'><br/></div><div class='line' id='LC1061'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Initiate network connections</span></div><div class='line' id='LC1062'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nStart</span> <span class="o">=</span> <span class="n">GetTime</span><span class="p">();</span></div><div class='line' id='LC1063'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">loop</span></div><div class='line' id='LC1064'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1065'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Limit outbound connections</span></div><div class='line' id='LC1066'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC1067'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">500</span><span class="p">);</span></div><div class='line' id='LC1068'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">loop</span></div><div class='line' id='LC1069'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1070'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nOutbound</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC1071'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC1072'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC1073'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fInbound</span><span class="p">)</span></div><div class='line' id='LC1074'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nOutbound</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC1075'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nMaxOutboundConnections</span> <span class="o">=</span> <span class="n">MAX_OUTBOUND_CONNECTIONS</span><span class="p">;</span></div><div class='line' id='LC1076'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nMaxOutboundConnections</span> <span class="o">=</span> <span class="n">min</span><span class="p">(</span><span class="n">nMaxOutboundConnections</span><span class="p">,</span> <span class="p">(</span><span class="kt">int</span><span class="p">)</span><span class="n">GetArg</span><span class="p">(</span><span class="s">&quot;-maxconnections&quot;</span><span class="p">,</span> <span class="mi">125</span><span class="p">));</span></div><div class='line' id='LC1077'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nOutbound</span> <span class="o">&lt;</span> <span class="n">nMaxOutboundConnections</span><span class="p">)</span></div><div class='line' id='LC1078'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">break</span><span class="p">;</span></div><div class='line' id='LC1079'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">2000</span><span class="p">);</span></div><div class='line' id='LC1080'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1081'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC1082'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1083'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC1084'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1085'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC1086'><br/></div><div class='line' id='LC1087'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_mapAddresses</span><span class="p">)</span></div><div class='line' id='LC1088'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1089'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Add seed nodes if IRC isn&#39;t working</span></div><div class='line' id='LC1090'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">static</span> <span class="kt">bool</span> <span class="n">fSeedUsed</span><span class="p">;</span></div><div class='line' id='LC1091'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">bool</span> <span class="n">fTOR</span> <span class="o">=</span> <span class="p">(</span><span class="n">fUseProxy</span> <span class="o">&amp;&amp;</span> <span class="n">addrProxy</span><span class="p">.</span><span class="n">port</span> <span class="o">==</span> <span class="n">htons</span><span class="p">(</span><span class="mi">9050</span><span class="p">));</span></div><div class='line' id='LC1092'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">mapAddresses</span><span class="p">.</span><span class="n">empty</span><span class="p">()</span> <span class="o">&amp;&amp;</span> <span class="p">(</span><span class="n">GetTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">nStart</span> <span class="o">&gt;</span> <span class="mi">60</span> <span class="o">||</span> <span class="n">fTOR</span><span class="p">)</span> <span class="o">&amp;&amp;</span> <span class="o">!</span><span class="n">fTestNet</span><span class="p">)</span></div><div class='line' id='LC1093'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1094'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">i</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="n">i</span> <span class="o">&lt;</span> <span class="n">ARRAYLEN</span><span class="p">(</span><span class="n">pnSeed</span><span class="p">);</span> <span class="n">i</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC1095'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1096'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// It&#39;ll only connect to one or two seed nodes because once it connects,</span></div><div class='line' id='LC1097'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// it&#39;ll get a pile of addresses with newer timestamps.</span></div><div class='line' id='LC1098'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addr</span><span class="p">;</span></div><div class='line' id='LC1099'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addr</span><span class="p">.</span><span class="n">ip</span> <span class="o">=</span> <span class="n">pnSeed</span><span class="p">[</span><span class="n">i</span><span class="p">];</span></div><div class='line' id='LC1100'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addr</span><span class="p">.</span><span class="n">nTime</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC1101'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">AddAddress</span><span class="p">(</span><span class="n">addr</span><span class="p">);</span></div><div class='line' id='LC1102'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1103'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fSeedUsed</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC1104'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1105'><br/></div><div class='line' id='LC1106'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fSeedUsed</span> <span class="o">&amp;&amp;</span> <span class="n">mapAddresses</span><span class="p">.</span><span class="n">size</span><span class="p">()</span> <span class="o">&gt;</span> <span class="n">ARRAYLEN</span><span class="p">(</span><span class="n">pnSeed</span><span class="p">)</span> <span class="o">+</span> <span class="mi">100</span><span class="p">)</span></div><div class='line' id='LC1107'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1108'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Disconnect seed nodes</span></div><div class='line' id='LC1109'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">set</span><span class="o">&lt;</span><span class="kt">unsigned</span> <span class="kt">int</span><span class="o">&gt;</span> <span class="n">setSeed</span><span class="p">(</span><span class="n">pnSeed</span><span class="p">,</span> <span class="n">pnSeed</span> <span class="o">+</span> <span class="n">ARRAYLEN</span><span class="p">(</span><span class="n">pnSeed</span><span class="p">));</span></div><div class='line' id='LC1110'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">static</span> <span class="n">int64</span> <span class="n">nSeedDisconnected</span><span class="p">;</span></div><div class='line' id='LC1111'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nSeedDisconnected</span> <span class="o">==</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC1112'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1113'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nSeedDisconnected</span> <span class="o">=</span> <span class="n">GetTime</span><span class="p">();</span></div><div class='line' id='LC1114'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC1115'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC1116'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">setSeed</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">addr</span><span class="p">.</span><span class="n">ip</span><span class="p">))</span></div><div class='line' id='LC1117'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fDisconnect</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC1118'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1119'><br/></div><div class='line' id='LC1120'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Keep setting timestamps to 0 so they won&#39;t reconnect</span></div><div class='line' id='LC1121'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">nSeedDisconnected</span> <span class="o">&lt;</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span><span class="p">)</span></div><div class='line' id='LC1122'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1123'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">PAIRTYPE</span><span class="p">(</span><span class="k">const</span> <span class="n">vector</span><span class="o">&lt;</span><span class="kt">unsigned</span> <span class="kt">char</span><span class="o">&gt;</span><span class="p">,</span> <span class="n">CAddress</span><span class="p">)</span><span class="o">&amp;</span> <span class="n">item</span><span class="p">,</span> <span class="n">mapAddresses</span><span class="p">)</span></div><div class='line' id='LC1124'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1125'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">setSeed</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="n">item</span><span class="p">.</span><span class="n">second</span><span class="p">.</span><span class="n">ip</span><span class="p">)</span> <span class="o">&amp;&amp;</span> <span class="n">item</span><span class="p">.</span><span class="n">second</span><span class="p">.</span><span class="n">nTime</span> <span class="o">!=</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC1126'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1127'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">item</span><span class="p">.</span><span class="n">second</span><span class="p">.</span><span class="n">nTime</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span></div><div class='line' id='LC1128'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddrDB</span><span class="p">().</span><span class="n">WriteAddress</span><span class="p">(</span><span class="n">item</span><span class="p">.</span><span class="n">second</span><span class="p">);</span></div><div class='line' id='LC1129'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1130'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1131'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1132'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1133'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1134'><br/></div><div class='line' id='LC1135'><br/></div><div class='line' id='LC1136'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC1137'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Choose an address to connect to based on most recently seen</span></div><div class='line' id='LC1138'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC1139'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addrConnect</span><span class="p">;</span></div><div class='line' id='LC1140'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nBest</span> <span class="o">=</span> <span class="n">INT64_MIN</span><span class="p">;</span></div><div class='line' id='LC1141'><br/></div><div class='line' id='LC1142'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Only connect to one address per a.b.?.? range.</span></div><div class='line' id='LC1143'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Do this here so we don&#39;t have to critsect vNodes inside mapAddresses critsect.</span></div><div class='line' id='LC1144'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">set</span><span class="o">&lt;</span><span class="kt">unsigned</span> <span class="kt">int</span><span class="o">&gt;</span> <span class="n">setConnected</span><span class="p">;</span></div><div class='line' id='LC1145'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC1146'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC1147'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">setConnected</span><span class="p">.</span><span class="n">insert</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">addr</span><span class="p">.</span><span class="n">ip</span> <span class="o">&amp;</span> <span class="mh">0x0000ffff</span><span class="p">);</span></div><div class='line' id='LC1148'><br/></div><div class='line' id='LC1149'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_mapAddresses</span><span class="p">)</span></div><div class='line' id='LC1150'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1151'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="k">const</span> <span class="n">PAIRTYPE</span><span class="p">(</span><span class="n">vector</span><span class="o">&lt;</span><span class="kt">unsigned</span> <span class="kt">char</span><span class="o">&gt;</span><span class="p">,</span> <span class="n">CAddress</span><span class="p">)</span><span class="o">&amp;</span> <span class="n">item</span><span class="p">,</span> <span class="n">mapAddresses</span><span class="p">)</span></div><div class='line' id='LC1152'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1153'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">const</span> <span class="n">CAddress</span><span class="o">&amp;</span> <span class="n">addr</span> <span class="o">=</span> <span class="n">item</span><span class="p">.</span><span class="n">second</span><span class="p">;</span></div><div class='line' id='LC1154'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">addr</span><span class="p">.</span><span class="n">IsIPv4</span><span class="p">()</span> <span class="o">||</span> <span class="o">!</span><span class="n">addr</span><span class="p">.</span><span class="n">IsValid</span><span class="p">()</span> <span class="o">||</span> <span class="n">setConnected</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">ip</span> <span class="o">&amp;</span> <span class="mh">0x0000ffff</span><span class="p">))</span></div><div class='line' id='LC1155'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC1156'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nSinceLastSeen</span> <span class="o">=</span> <span class="n">GetAdjustedTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">addr</span><span class="p">.</span><span class="n">nTime</span><span class="p">;</span></div><div class='line' id='LC1157'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nSinceLastTry</span> <span class="o">=</span> <span class="n">GetAdjustedTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">addr</span><span class="p">.</span><span class="n">nLastTry</span><span class="p">;</span></div><div class='line' id='LC1158'><br/></div><div class='line' id='LC1159'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Randomize the order in a deterministic way, putting the standard port first</span></div><div class='line' id='LC1160'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nRandomizer</span> <span class="o">=</span> <span class="p">(</span><span class="n">uint64</span><span class="p">)(</span><span class="n">nStart</span> <span class="o">*</span> <span class="mi">4951</span> <span class="o">+</span> <span class="n">addr</span><span class="p">.</span><span class="n">nLastTry</span> <span class="o">*</span> <span class="mi">9567851</span> <span class="o">+</span> <span class="n">addr</span><span class="p">.</span><span class="n">ip</span> <span class="o">*</span> <span class="mi">7789</span><span class="p">)</span> <span class="o">%</span> <span class="p">(</span><span class="mi">2</span> <span class="o">*</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span><span class="p">);</span></div><div class='line' id='LC1161'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">port</span> <span class="o">!=</span> <span class="n">GetDefaultPort</span><span class="p">())</span></div><div class='line' id='LC1162'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nRandomizer</span> <span class="o">+=</span> <span class="mi">2</span> <span class="o">*</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span><span class="p">;</span></div><div class='line' id='LC1163'><br/></div><div class='line' id='LC1164'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Last seen  Base retry frequency</span></div><div class='line' id='LC1165'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//   &lt;1 hour   10 min</span></div><div class='line' id='LC1166'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//    1 hour    1 hour</span></div><div class='line' id='LC1167'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//    4 hours   2 hours</span></div><div class='line' id='LC1168'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//   24 hours   5 hours</span></div><div class='line' id='LC1169'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//   48 hours   7 hours</span></div><div class='line' id='LC1170'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//    7 days   13 hours</span></div><div class='line' id='LC1171'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//   30 days   27 hours</span></div><div class='line' id='LC1172'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//   90 days   46 hours</span></div><div class='line' id='LC1173'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//  365 days   93 hours</span></div><div class='line' id='LC1174'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nDelay</span> <span class="o">=</span> <span class="p">(</span><span class="n">int64</span><span class="p">)(</span><span class="mf">3600.0</span> <span class="o">*</span> <span class="n">sqrt</span><span class="p">(</span><span class="n">fabs</span><span class="p">((</span><span class="kt">double</span><span class="p">)</span><span class="n">nSinceLastSeen</span><span class="p">)</span> <span class="o">/</span> <span class="mf">3600.0</span><span class="p">)</span> <span class="o">+</span> <span class="n">nRandomizer</span><span class="p">);</span></div><div class='line' id='LC1175'><br/></div><div class='line' id='LC1176'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Fast reconnect for one hour after last seen</span></div><div class='line' id='LC1177'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nSinceLastSeen</span> <span class="o">&lt;</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span><span class="p">)</span></div><div class='line' id='LC1178'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nDelay</span> <span class="o">=</span> <span class="mi">10</span> <span class="o">*</span> <span class="mi">60</span><span class="p">;</span></div><div class='line' id='LC1179'><br/></div><div class='line' id='LC1180'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Limit retry frequency</span></div><div class='line' id='LC1181'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nSinceLastTry</span> <span class="o">&lt;</span> <span class="n">nDelay</span><span class="p">)</span></div><div class='line' id='LC1182'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC1183'><br/></div><div class='line' id='LC1184'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// If we have IRC, we&#39;ll be notified when they first come online,</span></div><div class='line' id='LC1185'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// and again every 24 hours by the refresh broadcast.</span></div><div class='line' id='LC1186'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nGotIRCAddresses</span> <span class="o">&gt;</span> <span class="mi">0</span> <span class="o">&amp;&amp;</span> <span class="n">vNodes</span><span class="p">.</span><span class="n">size</span><span class="p">()</span> <span class="o">&gt;=</span> <span class="mi">2</span> <span class="o">&amp;&amp;</span> <span class="n">nSinceLastSeen</span> <span class="o">&gt;</span> <span class="mi">24</span> <span class="o">*</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span><span class="p">)</span></div><div class='line' id='LC1187'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC1188'><br/></div><div class='line' id='LC1189'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Only try the old stuff if we don&#39;t have enough connections</span></div><div class='line' id='LC1190'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vNodes</span><span class="p">.</span><span class="n">size</span><span class="p">()</span> <span class="o">&gt;=</span> <span class="mi">8</span> <span class="o">&amp;&amp;</span> <span class="n">nSinceLastSeen</span> <span class="o">&gt;</span> <span class="mi">24</span> <span class="o">*</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span><span class="p">)</span></div><div class='line' id='LC1191'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC1192'><br/></div><div class='line' id='LC1193'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// If multiple addresses are ready, prioritize by time since</span></div><div class='line' id='LC1194'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// last seen and time since last tried.</span></div><div class='line' id='LC1195'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nScore</span> <span class="o">=</span> <span class="n">min</span><span class="p">(</span><span class="n">nSinceLastTry</span><span class="p">,</span> <span class="p">(</span><span class="n">int64</span><span class="p">)</span><span class="mi">24</span> <span class="o">*</span> <span class="mi">60</span> <span class="o">*</span> <span class="mi">60</span><span class="p">)</span> <span class="o">-</span> <span class="n">nSinceLastSeen</span> <span class="o">-</span> <span class="n">nRandomizer</span><span class="p">;</span></div><div class='line' id='LC1196'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nScore</span> <span class="o">&gt;</span> <span class="n">nBest</span><span class="p">)</span></div><div class='line' id='LC1197'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1198'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nBest</span> <span class="o">=</span> <span class="n">nScore</span><span class="p">;</span></div><div class='line' id='LC1199'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrConnect</span> <span class="o">=</span> <span class="n">addr</span><span class="p">;</span></div><div class='line' id='LC1200'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1201'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1202'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1203'><br/></div><div class='line' id='LC1204'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">IsValid</span><span class="p">())</span></div><div class='line' id='LC1205'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">OpenNetworkConnection</span><span class="p">(</span><span class="n">addrConnect</span><span class="p">);</span></div><div class='line' id='LC1206'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1207'><span class="p">}</span></div><div class='line' id='LC1208'><br/></div><div class='line' id='LC1209'><span class="kt">bool</span> <span class="n">OpenNetworkConnection</span><span class="p">(</span><span class="k">const</span> <span class="n">CAddress</span><span class="o">&amp;</span> <span class="n">addrConnect</span><span class="p">)</span></div><div class='line' id='LC1210'><span class="p">{</span></div><div class='line' id='LC1211'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC1212'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Initiate outbound network connection</span></div><div class='line' id='LC1213'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC1214'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1215'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC1216'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">ip</span> <span class="o">==</span> <span class="n">addrLocalHost</span><span class="p">.</span><span class="n">ip</span> <span class="o">||</span> <span class="o">!</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">IsIPv4</span><span class="p">()</span> <span class="o">||</span> <span class="n">FindNode</span><span class="p">(</span><span class="n">addrConnect</span><span class="p">.</span><span class="n">ip</span><span class="p">))</span></div><div class='line' id='LC1217'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC1218'><br/></div><div class='line' id='LC1219'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC1220'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span> <span class="o">=</span> <span class="n">ConnectNode</span><span class="p">(</span><span class="n">addrConnect</span><span class="p">);</span></div><div class='line' id='LC1221'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC1222'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1223'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC1224'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">pnode</span><span class="p">)</span></div><div class='line' id='LC1225'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC1226'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">fNetworkNode</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC1227'><br/></div><div class='line' id='LC1228'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC1229'><span class="p">}</span></div><div class='line' id='LC1230'><br/></div><div class='line' id='LC1231'><br/></div><div class='line' id='LC1232'><br/></div><div class='line' id='LC1233'><br/></div><div class='line' id='LC1234'><br/></div><div class='line' id='LC1235'><br/></div><div class='line' id='LC1236'><br/></div><div class='line' id='LC1237'><br/></div><div class='line' id='LC1238'><span class="kt">void</span> <span class="n">ThreadMessageHandler</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC1239'><span class="p">{</span></div><div class='line' id='LC1240'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">IMPLEMENT_RANDOMIZE_STACK</span><span class="p">(</span><span class="n">ThreadMessageHandler</span><span class="p">(</span><span class="n">parg</span><span class="p">));</span></div><div class='line' id='LC1241'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">try</span></div><div class='line' id='LC1242'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1243'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">2</span><span class="p">]</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC1244'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ThreadMessageHandler2</span><span class="p">(</span><span class="n">parg</span><span class="p">);</span></div><div class='line' id='LC1245'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">2</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC1246'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1247'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">catch</span> <span class="p">(</span><span class="n">std</span><span class="o">::</span><span class="n">exception</span><span class="o">&amp;</span> <span class="n">e</span><span class="p">)</span> <span class="p">{</span></div><div class='line' id='LC1248'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">2</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC1249'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintException</span><span class="p">(</span><span class="o">&amp;</span><span class="n">e</span><span class="p">,</span> <span class="s">&quot;ThreadMessageHandler()&quot;</span><span class="p">);</span></div><div class='line' id='LC1250'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span> <span class="k">catch</span> <span class="p">(...)</span> <span class="p">{</span></div><div class='line' id='LC1251'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">2</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC1252'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">PrintException</span><span class="p">(</span><span class="nb">NULL</span><span class="p">,</span> <span class="s">&quot;ThreadMessageHandler()&quot;</span><span class="p">);</span></div><div class='line' id='LC1253'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1254'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadMessageHandler exiting</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1255'><span class="p">}</span></div><div class='line' id='LC1256'><br/></div><div class='line' id='LC1257'><span class="kt">void</span> <span class="n">ThreadMessageHandler2</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC1258'><span class="p">{</span></div><div class='line' id='LC1259'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadMessageHandler started</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1260'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">SetThreadPriority</span><span class="p">(</span><span class="n">THREAD_PRIORITY_BELOW_NORMAL</span><span class="p">);</span></div><div class='line' id='LC1261'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">while</span> <span class="p">(</span><span class="o">!</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1262'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1263'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vector</span><span class="o">&lt;</span><span class="n">CNode</span><span class="o">*&gt;</span> <span class="n">vNodesCopy</span><span class="p">;</span></div><div class='line' id='LC1264'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC1265'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1266'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vNodesCopy</span> <span class="o">=</span> <span class="n">vNodes</span><span class="p">;</span></div><div class='line' id='LC1267'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodesCopy</span><span class="p">)</span></div><div class='line' id='LC1268'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">AddRef</span><span class="p">();</span></div><div class='line' id='LC1269'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1270'><br/></div><div class='line' id='LC1271'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Poll the connected nodes for messages</span></div><div class='line' id='LC1272'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CNode</span><span class="o">*</span> <span class="n">pnodeTrickle</span> <span class="o">=</span> <span class="nb">NULL</span><span class="p">;</span></div><div class='line' id='LC1273'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">vNodesCopy</span><span class="p">.</span><span class="n">empty</span><span class="p">())</span></div><div class='line' id='LC1274'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnodeTrickle</span> <span class="o">=</span> <span class="n">vNodesCopy</span><span class="p">[</span><span class="n">GetRand</span><span class="p">(</span><span class="n">vNodesCopy</span><span class="p">.</span><span class="n">size</span><span class="p">())];</span></div><div class='line' id='LC1275'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodesCopy</span><span class="p">)</span></div><div class='line' id='LC1276'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1277'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Receive messages</span></div><div class='line' id='LC1278'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">TRY_CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_vRecv</span><span class="p">)</span></div><div class='line' id='LC1279'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">ProcessMessages</span><span class="p">(</span><span class="n">pnode</span><span class="p">);</span></div><div class='line' id='LC1280'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1281'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC1282'><br/></div><div class='line' id='LC1283'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Send messages</span></div><div class='line' id='LC1284'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">TRY_CRITICAL_BLOCK</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">cs_vSend</span><span class="p">)</span></div><div class='line' id='LC1285'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">SendMessages</span><span class="p">(</span><span class="n">pnode</span><span class="p">,</span> <span class="n">pnode</span> <span class="o">==</span> <span class="n">pnodeTrickle</span><span class="p">);</span></div><div class='line' id='LC1286'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1287'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC1288'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1289'><br/></div><div class='line' id='LC1290'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CRITICAL_BLOCK</span><span class="p">(</span><span class="n">cs_vNodes</span><span class="p">)</span></div><div class='line' id='LC1291'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1292'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodesCopy</span><span class="p">)</span></div><div class='line' id='LC1293'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnode</span><span class="o">-&gt;</span><span class="n">Release</span><span class="p">();</span></div><div class='line' id='LC1294'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1295'><br/></div><div class='line' id='LC1296'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Wait and allow messages to bunch up.</span></div><div class='line' id='LC1297'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Reduce vnThreadsRunning so StopNode has permission to exit while</span></div><div class='line' id='LC1298'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// we&#39;re sleeping, but we must always check fShutdown after doing this.</span></div><div class='line' id='LC1299'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">2</span><span class="p">]</span><span class="o">--</span><span class="p">;</span></div><div class='line' id='LC1300'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">100</span><span class="p">);</span></div><div class='line' id='LC1301'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fRequestShutdown</span><span class="p">)</span></div><div class='line' id='LC1302'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Shutdown</span><span class="p">(</span><span class="nb">NULL</span><span class="p">);</span></div><div class='line' id='LC1303'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">2</span><span class="p">]</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC1304'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fShutdown</span><span class="p">)</span></div><div class='line' id='LC1305'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span><span class="p">;</span></div><div class='line' id='LC1306'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1307'><span class="p">}</span></div><div class='line' id='LC1308'><br/></div><div class='line' id='LC1309'><br/></div><div class='line' id='LC1310'><br/></div><div class='line' id='LC1311'><br/></div><div class='line' id='LC1312'><br/></div><div class='line' id='LC1313'><br/></div><div class='line' id='LC1314'><br/></div><div class='line' id='LC1315'><br/></div><div class='line' id='LC1316'><br/></div><div class='line' id='LC1317'><span class="kt">bool</span> <span class="n">BindListenPort</span><span class="p">(</span><span class="n">string</span><span class="o">&amp;</span> <span class="n">strError</span><span class="p">)</span></div><div class='line' id='LC1318'><span class="p">{</span></div><div class='line' id='LC1319'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strError</span> <span class="o">=</span> <span class="s">&quot;&quot;</span><span class="p">;</span></div><div class='line' id='LC1320'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nOne</span> <span class="o">=</span> <span class="mi">1</span><span class="p">;</span></div><div class='line' id='LC1321'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrLocalHost</span><span class="p">.</span><span class="n">port</span> <span class="o">=</span> <span class="n">GetDefaultPort</span><span class="p">();</span></div><div class='line' id='LC1322'><br/></div><div class='line' id='LC1323'><span class="cp">#ifdef __WXMSW__</span></div><div class='line' id='LC1324'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Initialize Windows Sockets</span></div><div class='line' id='LC1325'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">WSADATA</span> <span class="n">wsadata</span><span class="p">;</span></div><div class='line' id='LC1326'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">ret</span> <span class="o">=</span> <span class="n">WSAStartup</span><span class="p">(</span><span class="n">MAKEWORD</span><span class="p">(</span><span class="mi">2</span><span class="p">,</span><span class="mi">2</span><span class="p">),</span> <span class="o">&amp;</span><span class="n">wsadata</span><span class="p">);</span></div><div class='line' id='LC1327'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">ret</span> <span class="o">!=</span> <span class="n">NO_ERROR</span><span class="p">)</span></div><div class='line' id='LC1328'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1329'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strError</span> <span class="o">=</span> <span class="n">strprintf</span><span class="p">(</span><span class="s">&quot;Error: TCP/IP socket library failed to start (WSAStartup returned error %d)&quot;</span><span class="p">,</span> <span class="n">ret</span><span class="p">);</span></div><div class='line' id='LC1330'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;%s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">strError</span><span class="p">.</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC1331'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC1332'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1333'><span class="cp">#endif</span></div><div class='line' id='LC1334'><br/></div><div class='line' id='LC1335'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Create socket for listening for incoming connections</span></div><div class='line' id='LC1336'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">hListenSocket</span> <span class="o">=</span> <span class="n">socket</span><span class="p">(</span><span class="n">AF_INET</span><span class="p">,</span> <span class="n">SOCK_STREAM</span><span class="p">,</span> <span class="n">IPPROTO_TCP</span><span class="p">);</span></div><div class='line' id='LC1337'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">hListenSocket</span> <span class="o">==</span> <span class="n">INVALID_SOCKET</span><span class="p">)</span></div><div class='line' id='LC1338'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1339'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strError</span> <span class="o">=</span> <span class="n">strprintf</span><span class="p">(</span><span class="s">&quot;Error: Couldn&#39;t open socket for incoming connections (socket returned error %d)&quot;</span><span class="p">,</span> <span class="n">WSAGetLastError</span><span class="p">());</span></div><div class='line' id='LC1340'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;%s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">strError</span><span class="p">.</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC1341'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC1342'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1343'><br/></div><div class='line' id='LC1344'><span class="cp">#ifdef BSD</span></div><div class='line' id='LC1345'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Different way of disabling SIGPIPE on BSD</span></div><div class='line' id='LC1346'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">setsockopt</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">,</span> <span class="n">SOL_SOCKET</span><span class="p">,</span> <span class="n">SO_NOSIGPIPE</span><span class="p">,</span> <span class="p">(</span><span class="kt">void</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="n">nOne</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="kt">int</span><span class="p">));</span></div><div class='line' id='LC1347'><span class="cp">#endif</span></div><div class='line' id='LC1348'><br/></div><div class='line' id='LC1349'><span class="cp">#ifndef __WXMSW__</span></div><div class='line' id='LC1350'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Allow binding if the port is still in TIME_WAIT state after</span></div><div class='line' id='LC1351'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// the program was closed and restarted.  Not an issue on windows.</span></div><div class='line' id='LC1352'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">setsockopt</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">,</span> <span class="n">SOL_SOCKET</span><span class="p">,</span> <span class="n">SO_REUSEADDR</span><span class="p">,</span> <span class="p">(</span><span class="kt">void</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="n">nOne</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="kt">int</span><span class="p">));</span></div><div class='line' id='LC1353'><span class="cp">#endif</span></div><div class='line' id='LC1354'><br/></div><div class='line' id='LC1355'><span class="cp">#ifdef __WXMSW__</span></div><div class='line' id='LC1356'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Set to nonblocking, incoming connections will also inherit this</span></div><div class='line' id='LC1357'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">ioctlsocket</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">,</span> <span class="n">FIONBIO</span><span class="p">,</span> <span class="p">(</span><span class="n">u_long</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="n">nOne</span><span class="p">)</span> <span class="o">==</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC1358'><span class="cp">#else</span></div><div class='line' id='LC1359'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fcntl</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">,</span> <span class="n">F_SETFL</span><span class="p">,</span> <span class="n">O_NONBLOCK</span><span class="p">)</span> <span class="o">==</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC1360'><span class="cp">#endif</span></div><div class='line' id='LC1361'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1362'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strError</span> <span class="o">=</span> <span class="n">strprintf</span><span class="p">(</span><span class="s">&quot;Error: Couldn&#39;t set properties on socket for incoming connections (error %d)&quot;</span><span class="p">,</span> <span class="n">WSAGetLastError</span><span class="p">());</span></div><div class='line' id='LC1363'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;%s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">strError</span><span class="p">.</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC1364'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC1365'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1366'><br/></div><div class='line' id='LC1367'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// The sockaddr_in structure specifies the address family,</span></div><div class='line' id='LC1368'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// IP address, and port for the socket that is being bound</span></div><div class='line' id='LC1369'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">sockaddr_in</span> <span class="n">sockaddr</span><span class="p">;</span></div><div class='line' id='LC1370'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">memset</span><span class="p">(</span><span class="o">&amp;</span><span class="n">sockaddr</span><span class="p">,</span> <span class="mi">0</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">sockaddr</span><span class="p">));</span></div><div class='line' id='LC1371'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">sockaddr</span><span class="p">.</span><span class="n">sin_family</span> <span class="o">=</span> <span class="n">AF_INET</span><span class="p">;</span></div><div class='line' id='LC1372'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">sockaddr</span><span class="p">.</span><span class="n">sin_addr</span><span class="p">.</span><span class="n">s_addr</span> <span class="o">=</span> <span class="n">INADDR_ANY</span><span class="p">;</span> <span class="c1">// bind to all IPs on this computer</span></div><div class='line' id='LC1373'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">sockaddr</span><span class="p">.</span><span class="n">sin_port</span> <span class="o">=</span> <span class="n">GetDefaultPort</span><span class="p">();</span></div><div class='line' id='LC1374'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">::</span><span class="n">bind</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">,</span> <span class="p">(</span><span class="k">struct</span> <span class="n">sockaddr</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="n">sockaddr</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">sockaddr</span><span class="p">))</span> <span class="o">==</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC1375'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1376'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">int</span> <span class="n">nErr</span> <span class="o">=</span> <span class="n">WSAGetLastError</span><span class="p">();</span></div><div class='line' id='LC1377'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">nErr</span> <span class="o">==</span> <span class="n">WSAEADDRINUSE</span><span class="p">)</span></div><div class='line' id='LC1378'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strError</span> <span class="o">=</span> <span class="n">strprintf</span><span class="p">(</span><span class="n">_</span><span class="p">(</span><span class="s">&quot;Unable to bind to port %d on this computer.  Bitcoin is probably already running.&quot;</span><span class="p">),</span> <span class="n">ntohs</span><span class="p">(</span><span class="n">sockaddr</span><span class="p">.</span><span class="n">sin_port</span><span class="p">));</span></div><div class='line' id='LC1379'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC1380'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strError</span> <span class="o">=</span> <span class="n">strprintf</span><span class="p">(</span><span class="s">&quot;Error: Unable to bind to port %d on this computer (bind returned error %d)&quot;</span><span class="p">,</span> <span class="n">ntohs</span><span class="p">(</span><span class="n">sockaddr</span><span class="p">.</span><span class="n">sin_port</span><span class="p">),</span> <span class="n">nErr</span><span class="p">);</span></div><div class='line' id='LC1381'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;%s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">strError</span><span class="p">.</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC1382'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC1383'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1384'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Bound to port %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">ntohs</span><span class="p">(</span><span class="n">sockaddr</span><span class="p">.</span><span class="n">sin_port</span><span class="p">));</span></div><div class='line' id='LC1385'><br/></div><div class='line' id='LC1386'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Listen for incoming connections</span></div><div class='line' id='LC1387'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">listen</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">,</span> <span class="n">SOMAXCONN</span><span class="p">)</span> <span class="o">==</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC1388'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1389'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">strError</span> <span class="o">=</span> <span class="n">strprintf</span><span class="p">(</span><span class="s">&quot;Error: Listening for incoming connections failed (listen returned error %d)&quot;</span><span class="p">,</span> <span class="n">WSAGetLastError</span><span class="p">());</span></div><div class='line' id='LC1390'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;%s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">strError</span><span class="p">.</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC1391'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">false</span><span class="p">;</span></div><div class='line' id='LC1392'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1393'><br/></div><div class='line' id='LC1394'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC1395'><span class="p">}</span></div><div class='line' id='LC1396'><br/></div><div class='line' id='LC1397'><span class="kt">void</span> <span class="n">StartNode</span><span class="p">(</span><span class="kt">void</span><span class="o">*</span> <span class="n">parg</span><span class="p">)</span></div><div class='line' id='LC1398'><span class="p">{</span></div><div class='line' id='LC1399'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnodeLocalHost</span> <span class="o">==</span> <span class="nb">NULL</span><span class="p">)</span></div><div class='line' id='LC1400'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pnodeLocalHost</span> <span class="o">=</span> <span class="k">new</span> <span class="n">CNode</span><span class="p">(</span><span class="n">INVALID_SOCKET</span><span class="p">,</span> <span class="n">CAddress</span><span class="p">(</span><span class="s">&quot;127.0.0.1&quot;</span><span class="p">,</span> <span class="n">nLocalServices</span><span class="p">));</span></div><div class='line' id='LC1401'><br/></div><div class='line' id='LC1402'><span class="cp">#ifdef __WXMSW__</span></div><div class='line' id='LC1403'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Get local host ip</span></div><div class='line' id='LC1404'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span> <span class="n">pszHostName</span><span class="p">[</span><span class="mi">1000</span><span class="p">]</span> <span class="o">=</span> <span class="s">&quot;&quot;</span><span class="p">;</span></div><div class='line' id='LC1405'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">gethostname</span><span class="p">(</span><span class="n">pszHostName</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">pszHostName</span><span class="p">))</span> <span class="o">!=</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC1406'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1407'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">hostent</span><span class="o">*</span> <span class="n">phostent</span> <span class="o">=</span> <span class="n">gethostbyname</span><span class="p">(</span><span class="n">pszHostName</span><span class="p">);</span></div><div class='line' id='LC1408'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">phostent</span><span class="p">)</span></div><div class='line' id='LC1409'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1410'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Take the first IP that isn&#39;t loopback 127.x.x.x</span></div><div class='line' id='LC1411'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">i</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span><span class="p">[</span><span class="n">i</span><span class="p">]</span> <span class="o">!=</span> <span class="nb">NULL</span><span class="p">;</span> <span class="n">i</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC1412'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;host ip %d: %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">i</span><span class="p">,</span> <span class="n">CAddress</span><span class="p">(</span><span class="o">*</span><span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span><span class="o">*</span><span class="p">)</span><span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span><span class="p">[</span><span class="n">i</span><span class="p">]).</span><span class="n">ToStringIP</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC1413'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="kt">int</span> <span class="n">i</span> <span class="o">=</span> <span class="mi">0</span><span class="p">;</span> <span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span><span class="p">[</span><span class="n">i</span><span class="p">]</span> <span class="o">!=</span> <span class="nb">NULL</span><span class="p">;</span> <span class="n">i</span><span class="o">++</span><span class="p">)</span></div><div class='line' id='LC1414'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1415'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addr</span><span class="p">(</span><span class="o">*</span><span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span><span class="o">*</span><span class="p">)</span><span class="n">phostent</span><span class="o">-&gt;</span><span class="n">h_addr_list</span><span class="p">[</span><span class="n">i</span><span class="p">],</span> <span class="n">GetDefaultPort</span><span class="p">(),</span> <span class="n">nLocalServices</span><span class="p">);</span></div><div class='line' id='LC1416'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">IsValid</span><span class="p">()</span> <span class="o">&amp;&amp;</span> <span class="n">addr</span><span class="p">.</span><span class="n">GetByte</span><span class="p">(</span><span class="mi">3</span><span class="p">)</span> <span class="o">!=</span> <span class="mi">127</span><span class="p">)</span></div><div class='line' id='LC1417'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1418'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrLocalHost</span> <span class="o">=</span> <span class="n">addr</span><span class="p">;</span></div><div class='line' id='LC1419'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">break</span><span class="p">;</span></div><div class='line' id='LC1420'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1421'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1422'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1423'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1424'><span class="cp">#else</span></div><div class='line' id='LC1425'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Get local host ip</span></div><div class='line' id='LC1426'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">ifaddrs</span><span class="o">*</span> <span class="n">myaddrs</span><span class="p">;</span></div><div class='line' id='LC1427'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">getifaddrs</span><span class="p">(</span><span class="o">&amp;</span><span class="n">myaddrs</span><span class="p">)</span> <span class="o">==</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC1428'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1429'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">for</span> <span class="p">(</span><span class="k">struct</span> <span class="n">ifaddrs</span><span class="o">*</span> <span class="n">ifa</span> <span class="o">=</span> <span class="n">myaddrs</span><span class="p">;</span> <span class="n">ifa</span> <span class="o">!=</span> <span class="nb">NULL</span><span class="p">;</span> <span class="n">ifa</span> <span class="o">=</span> <span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_next</span><span class="p">)</span></div><div class='line' id='LC1430'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1431'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_addr</span> <span class="o">==</span> <span class="nb">NULL</span><span class="p">)</span> <span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC1432'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">((</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_flags</span> <span class="o">&amp;</span> <span class="n">IFF_UP</span><span class="p">)</span> <span class="o">==</span> <span class="mi">0</span><span class="p">)</span> <span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC1433'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">strcmp</span><span class="p">(</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_name</span><span class="p">,</span> <span class="s">&quot;lo&quot;</span><span class="p">)</span> <span class="o">==</span> <span class="mi">0</span><span class="p">)</span> <span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC1434'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">strcmp</span><span class="p">(</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_name</span><span class="p">,</span> <span class="s">&quot;lo0&quot;</span><span class="p">)</span> <span class="o">==</span> <span class="mi">0</span><span class="p">)</span> <span class="k">continue</span><span class="p">;</span></div><div class='line' id='LC1435'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="kt">char</span> <span class="n">pszIP</span><span class="p">[</span><span class="mi">100</span><span class="p">];</span></div><div class='line' id='LC1436'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_addr</span><span class="o">-&gt;</span><span class="n">sa_family</span> <span class="o">==</span> <span class="n">AF_INET</span><span class="p">)</span></div><div class='line' id='LC1437'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1438'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">sockaddr_in</span><span class="o">*</span> <span class="n">s4</span> <span class="o">=</span> <span class="p">(</span><span class="k">struct</span> <span class="n">sockaddr_in</span><span class="o">*</span><span class="p">)(</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_addr</span><span class="p">);</span></div><div class='line' id='LC1439'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">inet_ntop</span><span class="p">(</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_addr</span><span class="o">-&gt;</span><span class="n">sa_family</span><span class="p">,</span> <span class="p">(</span><span class="kt">void</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="p">(</span><span class="n">s4</span><span class="o">-&gt;</span><span class="n">sin_addr</span><span class="p">),</span> <span class="n">pszIP</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">pszIP</span><span class="p">))</span> <span class="o">!=</span> <span class="nb">NULL</span><span class="p">)</span></div><div class='line' id='LC1440'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ipv4 %s: %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_name</span><span class="p">,</span> <span class="n">pszIP</span><span class="p">);</span></div><div class='line' id='LC1441'><br/></div><div class='line' id='LC1442'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Take the first IP that isn&#39;t loopback 127.x.x.x</span></div><div class='line' id='LC1443'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CAddress</span> <span class="n">addr</span><span class="p">(</span><span class="o">*</span><span class="p">(</span><span class="kt">unsigned</span> <span class="kt">int</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="n">s4</span><span class="o">-&gt;</span><span class="n">sin_addr</span><span class="p">,</span> <span class="n">GetDefaultPort</span><span class="p">(),</span> <span class="n">nLocalServices</span><span class="p">);</span></div><div class='line' id='LC1444'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">addr</span><span class="p">.</span><span class="n">IsValid</span><span class="p">()</span> <span class="o">&amp;&amp;</span> <span class="n">addr</span><span class="p">.</span><span class="n">GetByte</span><span class="p">(</span><span class="mi">3</span><span class="p">)</span> <span class="o">!=</span> <span class="mi">127</span><span class="p">)</span></div><div class='line' id='LC1445'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1446'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrLocalHost</span> <span class="o">=</span> <span class="n">addr</span><span class="p">;</span></div><div class='line' id='LC1447'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">break</span><span class="p">;</span></div><div class='line' id='LC1448'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1449'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1450'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span> <span class="k">if</span> <span class="p">(</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_addr</span><span class="o">-&gt;</span><span class="n">sa_family</span> <span class="o">==</span> <span class="n">AF_INET6</span><span class="p">)</span></div><div class='line' id='LC1451'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1452'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">struct</span> <span class="n">sockaddr_in6</span><span class="o">*</span> <span class="n">s6</span> <span class="o">=</span> <span class="p">(</span><span class="k">struct</span> <span class="n">sockaddr_in6</span><span class="o">*</span><span class="p">)(</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_addr</span><span class="p">);</span></div><div class='line' id='LC1453'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">inet_ntop</span><span class="p">(</span><span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_addr</span><span class="o">-&gt;</span><span class="n">sa_family</span><span class="p">,</span> <span class="p">(</span><span class="kt">void</span><span class="o">*</span><span class="p">)</span><span class="o">&amp;</span><span class="p">(</span><span class="n">s6</span><span class="o">-&gt;</span><span class="n">sin6_addr</span><span class="p">),</span> <span class="n">pszIP</span><span class="p">,</span> <span class="k">sizeof</span><span class="p">(</span><span class="n">pszIP</span><span class="p">))</span> <span class="o">!=</span> <span class="nb">NULL</span><span class="p">)</span></div><div class='line' id='LC1454'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;ipv6 %s: %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">ifa</span><span class="o">-&gt;</span><span class="n">ifa_name</span><span class="p">,</span> <span class="n">pszIP</span><span class="p">);</span></div><div class='line' id='LC1455'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1456'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1457'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">freeifaddrs</span><span class="p">(</span><span class="n">myaddrs</span><span class="p">);</span></div><div class='line' id='LC1458'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1459'><span class="cp">#endif</span></div><div class='line' id='LC1460'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;addrLocalHost = %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">addrLocalHost</span><span class="p">.</span><span class="n">ToString</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC1461'><br/></div><div class='line' id='LC1462'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">fUseProxy</span> <span class="o">||</span> <span class="n">mapArgs</span><span class="p">.</span><span class="n">count</span><span class="p">(</span><span class="s">&quot;-connect&quot;</span><span class="p">)</span> <span class="o">||</span> <span class="n">fNoListen</span><span class="p">)</span></div><div class='line' id='LC1463'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1464'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Proxies can&#39;t take incoming connections</span></div><div class='line' id='LC1465'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">addrLocalHost</span><span class="p">.</span><span class="n">ip</span> <span class="o">=</span> <span class="n">CAddress</span><span class="p">(</span><span class="s">&quot;0.0.0.0&quot;</span><span class="p">).</span><span class="n">ip</span><span class="p">;</span></div><div class='line' id='LC1466'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;addrLocalHost = %s</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">addrLocalHost</span><span class="p">.</span><span class="n">ToString</span><span class="p">().</span><span class="n">c_str</span><span class="p">());</span></div><div class='line' id='LC1467'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1468'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">else</span></div><div class='line' id='LC1469'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1470'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CreateThread</span><span class="p">(</span><span class="n">ThreadGetMyExternalIP</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">);</span></div><div class='line' id='LC1471'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1472'><br/></div><div class='line' id='LC1473'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC1474'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Start threads</span></div><div class='line' id='LC1475'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">//</span></div><div class='line' id='LC1476'><br/></div><div class='line' id='LC1477'><span class="cp">#ifdef USE_UPNP</span></div><div class='line' id='LC1478'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Map ports with UPnP</span></div><div class='line' id='LC1479'><span class="cp">#if USE_UPNP</span></div><div class='line' id='LC1480'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-noupnp&quot;</span><span class="p">))</span></div><div class='line' id='LC1481'><span class="cp">#else</span></div><div class='line' id='LC1482'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetBoolArg</span><span class="p">(</span><span class="s">&quot;-upnp&quot;</span><span class="p">))</span></div><div class='line' id='LC1483'><span class="cp">#endif</span></div><div class='line' id='LC1484'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1485'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">CreateThread</span><span class="p">(</span><span class="n">ThreadMapPort</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">))</span></div><div class='line' id='LC1486'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Error: ThreadMapPort(ThreadMapPort) failed</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1487'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1488'><span class="cp">#endif</span></div><div class='line' id='LC1489'><br/></div><div class='line' id='LC1490'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Get addresses from IRC and advertise ours</span></div><div class='line' id='LC1491'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">CreateThread</span><span class="p">(</span><span class="n">ThreadIRCSeed</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">))</span></div><div class='line' id='LC1492'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Error: CreateThread(ThreadIRCSeed) failed</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1493'><br/></div><div class='line' id='LC1494'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Send and receive from sockets, accept connections</span></div><div class='line' id='LC1495'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">pthread_t</span> <span class="n">hThreadSocketHandler</span> <span class="o">=</span> <span class="n">CreateThread</span><span class="p">(</span><span class="n">ThreadSocketHandler</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">,</span> <span class="kc">true</span><span class="p">);</span></div><div class='line' id='LC1496'><br/></div><div class='line' id='LC1497'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Initiate outbound connections</span></div><div class='line' id='LC1498'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">CreateThread</span><span class="p">(</span><span class="n">ThreadOpenConnections</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">))</span></div><div class='line' id='LC1499'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Error: CreateThread(ThreadOpenConnections) failed</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1500'><br/></div><div class='line' id='LC1501'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Process messages</span></div><div class='line' id='LC1502'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="o">!</span><span class="n">CreateThread</span><span class="p">(</span><span class="n">ThreadMessageHandler</span><span class="p">,</span> <span class="nb">NULL</span><span class="p">))</span></div><div class='line' id='LC1503'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;Error: CreateThread(ThreadMessageHandler) failed</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1504'><br/></div><div class='line' id='LC1505'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Generate coins in the background</span></div><div class='line' id='LC1506'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">GenerateBitcoins</span><span class="p">(</span><span class="n">fGenerateBitcoins</span><span class="p">);</span></div><div class='line' id='LC1507'><span class="p">}</span></div><div class='line' id='LC1508'><br/></div><div class='line' id='LC1509'><span class="kt">bool</span> <span class="n">StopNode</span><span class="p">()</span></div><div class='line' id='LC1510'><span class="p">{</span></div><div class='line' id='LC1511'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;StopNode()</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1512'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">fShutdown</span> <span class="o">=</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC1513'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">nTransactionsUpdated</span><span class="o">++</span><span class="p">;</span></div><div class='line' id='LC1514'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">int64</span> <span class="n">nStart</span> <span class="o">=</span> <span class="n">GetTime</span><span class="p">();</span></div><div class='line' id='LC1515'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">while</span> <span class="p">(</span><span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">0</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span> <span class="o">||</span> <span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">2</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span> <span class="o">||</span> <span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">3</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span> <span class="o">||</span> <span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">4</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span></div><div class='line' id='LC1516'><span class="cp">#ifdef USE_UPNP</span></div><div class='line' id='LC1517'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="o">||</span> <span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">5</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span></div><div class='line' id='LC1518'><span class="cp">#endif</span></div><div class='line' id='LC1519'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">)</span></div><div class='line' id='LC1520'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1521'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">GetTime</span><span class="p">()</span> <span class="o">-</span> <span class="n">nStart</span> <span class="o">&gt;</span> <span class="mi">20</span><span class="p">)</span></div><div class='line' id='LC1522'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">break</span><span class="p">;</span></div><div class='line' id='LC1523'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">20</span><span class="p">);</span></div><div class='line' id='LC1524'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1525'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">0</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span> <span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadSocketHandler still running</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1526'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">1</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span> <span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadOpenConnections still running</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1527'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">2</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span> <span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadMessageHandler still running</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1528'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">3</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span> <span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadBitcoinMiner still running</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1529'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">4</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span> <span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadRPCServer still running</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1530'><span class="cp">#ifdef USE_UPNP</span></div><div class='line' id='LC1531'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">5</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span> <span class="n">printf</span><span class="p">(</span><span class="s">&quot;ThreadMapPort still running</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">);</span></div><div class='line' id='LC1532'><span class="cp">#endif</span></div><div class='line' id='LC1533'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">while</span> <span class="p">(</span><span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">2</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span> <span class="o">||</span> <span class="n">vnThreadsRunning</span><span class="p">[</span><span class="mi">4</span><span class="p">]</span> <span class="o">&gt;</span> <span class="mi">0</span><span class="p">)</span></div><div class='line' id='LC1534'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">20</span><span class="p">);</span></div><div class='line' id='LC1535'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">Sleep</span><span class="p">(</span><span class="mi">50</span><span class="p">);</span></div><div class='line' id='LC1536'><br/></div><div class='line' id='LC1537'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">return</span> <span class="kc">true</span><span class="p">;</span></div><div class='line' id='LC1538'><span class="p">}</span></div><div class='line' id='LC1539'><br/></div><div class='line' id='LC1540'><span class="k">class</span> <span class="nc">CNetCleanup</span></div><div class='line' id='LC1541'><span class="p">{</span></div><div class='line' id='LC1542'><span class="k">public</span><span class="o">:</span></div><div class='line' id='LC1543'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">CNetCleanup</span><span class="p">()</span></div><div class='line' id='LC1544'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1545'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1546'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="o">~</span><span class="n">CNetCleanup</span><span class="p">()</span></div><div class='line' id='LC1547'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">{</span></div><div class='line' id='LC1548'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Close sockets</span></div><div class='line' id='LC1549'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">foreach</span><span class="p">(</span><span class="n">CNode</span><span class="o">*</span> <span class="n">pnode</span><span class="p">,</span> <span class="n">vNodes</span><span class="p">)</span></div><div class='line' id='LC1550'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span> <span class="o">!=</span> <span class="n">INVALID_SOCKET</span><span class="p">)</span></div><div class='line' id='LC1551'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">closesocket</span><span class="p">(</span><span class="n">pnode</span><span class="o">-&gt;</span><span class="n">hSocket</span><span class="p">);</span></div><div class='line' id='LC1552'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">hListenSocket</span> <span class="o">!=</span> <span class="n">INVALID_SOCKET</span><span class="p">)</span></div><div class='line' id='LC1553'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="k">if</span> <span class="p">(</span><span class="n">closesocket</span><span class="p">(</span><span class="n">hListenSocket</span><span class="p">)</span> <span class="o">==</span> <span class="n">SOCKET_ERROR</span><span class="p">)</span></div><div class='line' id='LC1554'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">printf</span><span class="p">(</span><span class="s">&quot;closesocket(hListenSocket) failed with error %d</span><span class="se">\n</span><span class="s">&quot;</span><span class="p">,</span> <span class="n">WSAGetLastError</span><span class="p">());</span></div><div class='line' id='LC1555'><br/></div><div class='line' id='LC1556'><span class="cp">#ifdef __WXMSW__</span></div><div class='line' id='LC1557'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="c1">// Shutdown Windows Sockets</span></div><div class='line' id='LC1558'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="n">WSACleanup</span><span class="p">();</span></div><div class='line' id='LC1559'><span class="cp">#endif</span></div><div class='line' id='LC1560'>&nbsp;&nbsp;&nbsp;&nbsp;<span class="p">}</span></div><div class='line' id='LC1561'><span class="p">}</span></div><div class='line' id='LC1562'><span class="n">instance_of_cnetcleanup</span><span class="p">;</span></div><div class='line' id='LC1563'><br/></div></pre></div>
              
            
          </td>
        </tr>
      </table>
    
  </div>


          </div>
        </div>
      </div>
    </div>
  

  </div>


<div class="frame frame-loading" style="display:none;">
  <img src="/images/modules/ajax/big_spinner_336699.gif">
</div>

    </div>
  
      
    </div>

    <div id="footer" class="clearfix">
      <div class="site">
        <div class="sponsor">
          <a href="http://www.rackspace.com" class="logo">
            <img alt="Dedicated Server" src="https://assets0.github.com/images/modules/footer/rackspace_logo.png?v2?2b893794092cd44a789db64b3da95d1091187f55" />
          </a>
          Powered by the <a href="http://www.rackspace.com ">Dedicated
          Servers</a> and<br/> <a href="http://www.rackspacecloud.com">Cloud
          Computing</a> of Rackspace Hosting<span>&reg;</span>
        </div>

        <ul class="links">
          <li class="blog"><a href="https://github.com/blog">Blog</a></li>
          <li><a href="/login/multipass?to=http%3A%2F%2Fsupport.github.com">Support</a></li>
          <li><a href="https://github.com/training">Training</a></li>
          <li><a href="http://jobs.github.com">Job Board</a></li>
          <li><a href="http://shop.github.com">Shop</a></li>
          <li><a href="https://github.com/contact">Contact</a></li>
          <li><a href="http://develop.github.com">API</a></li>
          <li><a href="http://status.github.com">Status</a></li>
        </ul>
        <ul class="sosueme">
          <li class="main">&copy; 2011 <span id="_rrt" title="0.10426s from fe1.rs.github.com">GitHub</span> Inc. All rights reserved.</li>
          <li><a href="/site/terms">Terms of Service</a></li>
          <li><a href="/site/privacy">Privacy</a></li>
          <li><a href="https://github.com/security">Security</a></li>
        </ul>
      </div>
    </div><!-- /#footer -->

    
      
      
        <!-- current locale:  -->
        <div class="locales">
          <div class="site">

            <ul class="choices clearfix limited-locales">
              <li><span class="current">English</span></li>
              
                  <li><a rel="nofollow" href="?locale=de">Deutsch</a></li>
              
                  <li><a rel="nofollow" href="?locale=fr">Français</a></li>
              
                  <li><a rel="nofollow" href="?locale=ja">日本語</a></li>
              
                  <li><a rel="nofollow" href="?locale=pt-BR">Português (BR)</a></li>
              
                  <li><a rel="nofollow" href="?locale=ru">Русский</a></li>
              
                  <li><a rel="nofollow" href="?locale=zh">中文</a></li>
              
              <li class="all"><a href="#" class="minibutton btn-forward js-all-locales"><span><span class="icon"></span>See all available languages</span></a></li>
            </ul>

            <div class="all-locales clearfix">
              <h3>Your current locale selection: <strong>English</strong>. Choose another?</h3>
              
              
                <ul class="choices">
                  
                      <li><a rel="nofollow" href="?locale=en">English</a></li>
                  
                      <li><a rel="nofollow" href="?locale=af">Afrikaans</a></li>
                  
                      <li><a rel="nofollow" href="?locale=ca">Català</a></li>
                  
                      <li><a rel="nofollow" href="?locale=cs">Čeština</a></li>
                  
                      <li><a rel="nofollow" href="?locale=de">Deutsch</a></li>
                  
                </ul>
              
                <ul class="choices">
                  
                      <li><a rel="nofollow" href="?locale=es">Español</a></li>
                  
                      <li><a rel="nofollow" href="?locale=fr">Français</a></li>
                  
                      <li><a rel="nofollow" href="?locale=hr">Hrvatski</a></li>
                  
                      <li><a rel="nofollow" href="?locale=hu">Magyar</a></li>
                  
                      <li><a rel="nofollow" href="?locale=id">Indonesia</a></li>
                  
                </ul>
              
                <ul class="choices">
                  
                      <li><a rel="nofollow" href="?locale=it">Italiano</a></li>
                  
                      <li><a rel="nofollow" href="?locale=ja">日本語</a></li>
                  
                      <li><a rel="nofollow" href="?locale=nl">Nederlands</a></li>
                  
                      <li><a rel="nofollow" href="?locale=no">Norsk</a></li>
                  
                      <li><a rel="nofollow" href="?locale=pl">Polski</a></li>
                  
                </ul>
              
                <ul class="choices">
                  
                      <li><a rel="nofollow" href="?locale=pt-BR">Português (BR)</a></li>
                  
                      <li><a rel="nofollow" href="?locale=ru">Русский</a></li>
                  
                      <li><a rel="nofollow" href="?locale=sr">Српски</a></li>
                  
                      <li><a rel="nofollow" href="?locale=sv">Svenska</a></li>
                  
                      <li><a rel="nofollow" href="?locale=zh">中文</a></li>
                  
                </ul>
              
            </div>

          </div>
          <div class="fade"></div>
        </div>
      
    

    <script>window._auth_token = "cf1c0b8959733617e26844d243ba9e11c32dfb89"</script>
    

<div id="keyboard_shortcuts_pane" style="display:none">
  <h2>Keyboard Shortcuts <small><a href="#" class="js-see-all-keyboard-shortcuts">(see all)</a></small></h2>

  <div class="columns threecols">
    <div class="column first">
      <h3>Site wide shortcuts</h3>
      <dl class="keyboard-mappings">
        <dt>s</dt>
        <dd>Focus site search</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>?</dt>
        <dd>Bring up this help dialog</dd>
      </dl>
    </div><!-- /.column.first -->

    <div class="column middle" style='display:none'>
      <h3>Commit list</h3>
      <dl class="keyboard-mappings">
        <dt>j</dt>
        <dd>Move selected down</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>k</dt>
        <dd>Move selected up</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>t</dt>
        <dd>Open tree</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>p</dt>
        <dd>Open parent</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>c <em>or</em> o <em>or</em> enter</dt>
        <dd>Open commit</dd>
      </dl>
    </div><!-- /.column.first -->

    <div class="column last" style='display:none'>
      <h3>Pull request list</h3>
      <dl class="keyboard-mappings">
        <dt>j</dt>
        <dd>Move selected down</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>k</dt>
        <dd>Move selected up</dd>
      </dl>
      <dl class="keyboard-mappings">
        <dt>o <em>or</em> enter</dt>
        <dd>Open issue</dd>
      </dl>
    </div><!-- /.columns.last -->

  </div><!-- /.columns.equacols -->

  <div style='display:none'>
    <div class="rule"></div>

    <h3>Issues</h3>

    <div class="columns threecols">
      <div class="column first">
        <dl class="keyboard-mappings">
          <dt>j</dt>
          <dd>Move selected down</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>k</dt>
          <dd>Move selected up</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>x</dt>
          <dd>Toggle select target</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>o <em>or</em> enter</dt>
          <dd>Open issue</dd>
        </dl>
      </div><!-- /.column.first -->
      <div class="column middle">
        <dl class="keyboard-mappings">
          <dt>I</dt>
          <dd>Mark selected as read</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>U</dt>
          <dd>Mark selected as unread</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>e</dt>
          <dd>Close selected</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>y</dt>
          <dd>Remove selected from view</dd>
        </dl>
      </div><!-- /.column.middle -->
      <div class="column last">
        <dl class="keyboard-mappings">
          <dt>c</dt>
          <dd>Create issue</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>l</dt>
          <dd>Create label</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>i</dt>
          <dd>Back to inbox</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>u</dt>
          <dd>Back to issues</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>/</dt>
          <dd>Focus issues search</dd>
        </dl>
      </div>
    </div>
  </div>

  <div style='display:none'>
    <div class="rule"></div>

    <h3>Network Graph</h3>
    <div class="columns equacols">
      <div class="column first">
        <dl class="keyboard-mappings">
          <dt><span class="badmono">←</span> <em>or</em> h</dt>
          <dd>Scroll left</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt><span class="badmono">→</span> <em>or</em> l</dt>
          <dd>Scroll right</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt><span class="badmono">↑</span> <em>or</em> k</dt>
          <dd>Scroll up</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt><span class="badmono">↓</span> <em>or</em> j</dt>
          <dd>Scroll down</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>t</dt>
          <dd>Toggle visibility of head labels</dd>
        </dl>
      </div><!-- /.column.first -->
      <div class="column last">
        <dl class="keyboard-mappings">
          <dt>shift <span class="badmono">←</span> <em>or</em> shift h</dt>
          <dd>Scroll all the way left</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>shift <span class="badmono">→</span> <em>or</em> shift l</dt>
          <dd>Scroll all the way right</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>shift <span class="badmono">↑</span> <em>or</em> shift k</dt>
          <dd>Scroll all the way up</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>shift <span class="badmono">↓</span> <em>or</em> shift j</dt>
          <dd>Scroll all the way down</dd>
        </dl>
      </div><!-- /.column.last -->
    </div>
  </div>

  <div >
    <div class="rule"></div>

    <h3>Source Code Browsing</h3>
    <div class="columns threecols">
      <div class="column first">
        <dl class="keyboard-mappings">
          <dt>t</dt>
          <dd>Activates the file finder</dd>
        </dl>
        <dl class="keyboard-mappings">
          <dt>l</dt>
          <dd>Jump to line</dd>
        </dl>
      </div>
    </div>
  </div>

</div>
    

    <!--[if IE 8]>
    <script type="text/javascript" charset="utf-8">
      $(document.body).addClass("ie8")
    </script>
    <![endif]-->

    <!--[if IE 7]>
    <script type="text/javascript" charset="utf-8">
      $(document.body).addClass("ie7")
    </script>
    <![endif]-->

    
    <script type='text/javascript'></script>
    
  </body>
</html>

