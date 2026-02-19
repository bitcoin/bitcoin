import { useState, useEffect } from 'react'
import './App.css'

function App() {
  const [wasmLoaded, setWasmLoaded] = useState(false)
  const [greeting, setGreeting] = useState('')
  const [txId, setTxId] = useState('')
  const [pdfResult, setPdfResult] = useState('')
  const [error, setError] = useState('')

  // WASM module state - will be used when WASM integration is complete
  // eslint-disable-next-line no-unused-vars
  const [wasmModule, setWasmModule] = useState(null)

  // Load WASM module
  useEffect(() => {
    const loadWasm = async () => {
      try {
        // TODO: Update path to your actual WASM module location
        // Uncomment the following lines after building the WASM module:
        // const wasm = await import('../wasm-pkg/bitcoin_pdf_utils.js');
        // await wasm.default();
        // setWasmModule(wasm);
        // setWasmLoaded(true);
        
        // Placeholder: WASM not yet integrated
        setError('WASM module not yet integrated. Build it first with: cd ../pdf-utils/wasm && ./generate_wasm.sh')
      } catch (err) {
        setWasmLoaded(false)
        setError('Failed to load WASM module: ' + err.message)
      }
    }

    loadWasm()
  }, [])

  const handleGreet = () => {
    if (wasmModule) {
      const result = wasmModule.greet('Bitcoin User')
      setGreeting(result)
    }
  }

  const handleGeneratePdf = () => {
    if (!wasmModule) {
      setError('WASM module not loaded')
      return
    }

    // Security: Input validation
    if (!txId.trim()) {
      setError('Transaction ID cannot be empty')
      return
    }

    if (txId.length > 64) {
      setError('Transaction ID too long')
      return
    }

    if (!/^[0-9a-fA-F]+$/.test(txId)) {
      setError('Transaction ID must contain only hexadecimal characters')
      return
    }

    try {
      const result = wasmModule.generate_bitcoin_transaction_pdf(txId)
      setPdfResult(result)
      setError('')
    } catch (err) {
      setError('PDF generation failed: ' + err.message)
      setPdfResult('')
    }
  }

  return (
    <div className="App">
      <header className="App-header">
        <h1>Bitcoin Core - PDF Utils Demo</h1>
        <p className="status">
          WASM Status: {wasmLoaded ? '✅ Loaded' : '❌ Not Loaded'}
        </p>
        {error && <p className="error">{error}</p>}
      </header>

      <main className="App-main">
        <section className="demo-section">
          <h2>Greeting Demo</h2>
          <button onClick={handleGreet} disabled={!wasmLoaded}>
            Get Greeting
          </button>
          {greeting && <p className="result">{greeting}</p>}
        </section>

        <section className="demo-section">
          <h2>Transaction PDF Generator</h2>
          <div className="input-group">
            <label htmlFor="txId">Transaction ID (hex):</label>
            <input
              id="txId"
              type="text"
              value={txId}
              onChange={(e) => setTxId(e.target.value)}
              placeholder="abc123def456..."
              maxLength={64}
              disabled={!wasmLoaded}
            />
          </div>
          <button onClick={handleGeneratePdf} disabled={!wasmLoaded}>
            Generate PDF
          </button>
          {pdfResult && <p className="result">{pdfResult}</p>}
        </section>

        <section className="security-info">
          <h3>🔒 Security Features</h3>
          <ul>
            <li>Input validation for transaction IDs</li>
            <li>XSS protection through CSP headers</li>
            <li>WASM sandboxing for safe code execution</li>
            <li>Secure HTTPS-only deployment recommended</li>
          </ul>
        </section>
      </main>
    </div>
  )
}

export default App
