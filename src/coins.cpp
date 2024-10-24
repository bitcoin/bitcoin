using System;
using System.Collections.Generic;
using System.Linq;

namespace Bitcoin
{
    public class Coin
    {
        public CTxOut Out { get; set; }
        public bool FCoinBase { get; set; }
        public uint NHeight { get; set; }

        public Coin(CTxOut outIn, int nHeightIn, bool fCoinBaseIn)
        {
            Out = outIn;
            FCoinBase = fCoinBaseIn;
            NHeight = (uint)nHeightIn;
        }

        public Coin()
        {
            FCoinBase = false;
            NHeight = 0;
        }

        public bool IsCoinBase()
        {
            return FCoinBase;
        }

        public void Serialize(Stream s)
        {
            if (IsSpent())
                throw new InvalidOperationException("Cannot serialize a spent coin.");

            uint code = NHeight * 2 + (FCoinBase ? 1U : 0U);
            s.Write(BitConverter.GetBytes(code), 0, sizeof(uint));
            Out.Serialize(s);
        }

        public void Unserialize(Stream s)
        {
            byte[] codeBytes = new byte[sizeof(uint)];
            s.Read(codeBytes, 0, sizeof(uint));
            uint code = BitConverter.ToUInt32(codeBytes, 0);
            NHeight = code >> 1;
            FCoinBase = (code & 1) == 1;
            Out.Unserialize(s);
        }

        public bool IsSpent()
        {
            return Out.IsNull();
        }

        public void Clear()
        {
            Out.SetNull();
            FCoinBase = false;
            NHeight = 0;
        }

        public long DynamicMemoryUsage()
        {
            return Out.ScriptPubKey.Length;
        }
    }

    public class CCoinsView
    {
        public virtual bool GetCoin(COutPoint outpoint, out Coin coin)
        {
            coin = null;
            return false;
        }

        public virtual uint256 GetBestBlock()
        {
            return new uint256();
        }

        public virtual List<uint256> GetHeadBlocks()
        {
            return new List<uint256>();
        }

        public virtual bool BatchWrite(CoinsViewCacheCursor cursor, uint256 hashBlock)
        {
            return false;
        }

        public virtual CCoinsViewCursor Cursor()
        {
            return null;
        }

        public virtual bool HaveCoin(COutPoint outpoint)
        {
            return GetCoin(outpoint, out _);
        }
    }

    public class CCoinsViewBacked : CCoinsView
    {
        protected CCoinsView Base;

        public CCoinsViewBacked(CCoinsView viewIn)
        {
            Base = viewIn;
        }

        public override bool GetCoin(COutPoint outpoint, out Coin coin)
        {
            return Base.GetCoin(outpoint, out coin);
        }

        public override bool HaveCoin(COutPoint outpoint)
        {
            return Base.HaveCoin(outpoint);
        }

        public override uint256 GetBestBlock()
        {
            return Base.GetBestBlock();
        }

        public override List<uint256> GetHeadBlocks()
        {
            return Base.GetHeadBlocks();
        }

        public void SetBackend(CCoinsView viewIn)
        {
            Base = viewIn;
        }

        public override bool BatchWrite(CoinsViewCacheCursor cursor, uint256 hashBlock)
        {
            return Base.BatchWrite(cursor, hashBlock);
        }

        public override CCoinsViewCursor Cursor()
        {
            return Base.Cursor();
        }

        public override long EstimateSize()
        {
            return Base.EstimateSize();
        }
    }

    public class CCoinsViewCache : CCoinsViewBacked
    {
        private bool _deterministic;
        private uint256 _hashBlock;
        private CCoinsMap _cacheCoins;
        private long _cachedCoinsUsage;

        public CCoinsViewCache(CCoinsView baseIn, bool deterministic = false) : base(baseIn)
        {
            _deterministic = deterministic;
            _cacheCoins = new CCoinsMap();
        }

        public override bool GetCoin(COutPoint outpoint, out Coin coin)
        {
            if (_cacheCoins.TryGetValue(outpoint, out var entry))
            {
                coin = entry.Coin;
                return !coin.IsSpent();
            }

            if (Base.GetCoin(outpoint, out coin))
            {
                _cacheCoins[outpoint] = new CCoinsCacheEntry(coin);
                _cachedCoinsUsage += coin.DynamicMemoryUsage();
                return true;
            }

            return false;
        }

        public override bool HaveCoin(COutPoint outpoint)
        {
            return _cacheCoins.ContainsKey(outpoint) || Base.HaveCoin(outpoint);
        }

        public bool HaveCoinInCache(COutPoint outpoint)
        {
            return _cacheCoins.ContainsKey(outpoint);
        }

        public override uint256 GetBestBlock()
        {
            if (_hashBlock == null)
                _hashBlock = Base.GetBestBlock();
            return _hashBlock;
        }

        public void SetBestBlock(uint256 hashBlockIn)
        {
            _hashBlock = hashBlockIn;
        }

        public override bool BatchWrite(CoinsViewCacheCursor cursor, uint256 hashBlockIn)
        {
            foreach (var entry in cursor)
            {
                if (!entry.Value.IsDirty)
                    continue;

                if (!_cacheCoins.TryGetValue(entry.Key, out var cacheEntry))
                {
                    if (!(entry.Value.IsFresh && entry.Value.Coin.IsSpent()))
                    {
                        _cacheCoins[entry.Key] = new CCoinsCacheEntry(entry.Value.Coin);
                        _cachedCoinsUsage += entry.Value.Coin.DynamicMemoryUsage();
                    }
                }
                else
                {
                    if (entry.Value.IsFresh && !cacheEntry.Coin.IsSpent())
                        throw new InvalidOperationException("FRESH flag misapplied to coin that exists in parent cache");

                    if (cacheEntry.IsFresh && entry.Value.Coin.IsSpent())
                    {
                        _cachedCoinsUsage -= cacheEntry.Coin.DynamicMemoryUsage();
                        _cacheCoins.Remove(entry.Key);
                    }
                    else
                    {
                        _cachedCoinsUsage -= cacheEntry.Coin.DynamicMemoryUsage();
                        cacheEntry.Coin = entry.Value.Coin;
                        _cachedCoinsUsage += cacheEntry.Coin.DynamicMemoryUsage();
                    }
                }
            }

            _hashBlock = hashBlockIn;
            return true;
        }

        public bool Flush()
        {
            var cursor = new CoinsViewCacheCursor(_cachedCoinsUsage, _cacheCoins, true);
            bool success = Base.BatchWrite(cursor, _hashBlock);
            if (success)
            {
                _cacheCoins.Clear();
                ReallocateCache();
            }
            _cachedCoinsUsage = 0;
            return success;
        }

        public bool Sync()
        {
            var cursor = new CoinsViewCacheCursor(_cachedCoinsUsage, _cacheCoins, false);
            bool success = Base.BatchWrite(cursor, _hashBlock);
            if (success)
            {
                if (_cacheCoins.Any(entry => entry.Value.IsDirty || entry.Value.IsFresh))
                    throw new InvalidOperationException("Not all unspent flagged entries were cleared");
            }
            return success;
        }

        public void Uncache(COutPoint outpoint)
        {
            if (_cacheCoins.TryGetValue(outpoint, out var entry) && !entry.IsDirty && !entry.IsFresh)
            {
                _cachedCoinsUsage -= entry.Coin.DynamicMemoryUsage();
                _cacheCoins.Remove(outpoint);
            }
        }

        public int GetCacheSize()
        {
            return _cacheCoins.Count;
        }

        public long DynamicMemoryUsage()
        {
            return _cacheCoins.Sum(entry => entry.Value.Coin.DynamicMemoryUsage()) + _cachedCoinsUsage;
        }

        public bool HaveInputs(CTransaction tx)
        {
            if (!tx.IsCoinBase)
            {
                foreach (var input in tx.Vin)
                {
                    if (!HaveCoin(input.Prevout))
                        return false;
                }
            }
            return true;
        }

        public void ReallocateCache()
        {
            if (_cacheCoins.Count != 0)
                throw new InvalidOperationException("Cache should be empty when reallocating.");

            _cacheCoins = new CCoinsMap();
        }

        public void SanityCheck()
        {
            long recomputedUsage = 0;
            int countFlagged = 0;

            foreach (var entry in _cacheCoins)
            {
                int attr = 0;
                if (entry.Value.IsDirty) attr |= 1;
                if (entry.Value.IsFresh) attr |= 2;
                if (entry.Value.Coin.IsSpent()) attr |= 4;

                if (attr == 2 || attr == 4 || attr == 7)
                    throw new InvalidOperationException("Invalid cache entry state.");

                recomputedUsage += entry.Value.Coin.DynamicMemoryUsage();

                if (entry.Value.IsDirty || entry.Value.IsFresh)
                    countFlagged++;
            }

            int countLinked = 0;
            foreach (var entry in _cacheCoins)
            {
                if (entry.Value.IsDirty || entry.Value.IsFresh)
                    countLinked++;
            }

            if (countLinked != countFlagged)
                throw new InvalidOperationException("Linked list count mismatch.");

            if (recomputedUsage != _cachedCoinsUsage)
                throw new InvalidOperationException("Cached coins usage mismatch.");
        }
    }

    public class CCoinsCacheEntry
    {
        public Coin Coin { get; set; }
        public bool IsDirty { get; set; }
        public bool IsFresh { get; set; }

        public CCoinsCacheEntry(Coin coin)
        {
            Coin = coin;
            IsDirty = false;
            IsFresh = false;
        }
    }

    public class CCoinsMap : Dictionary<COutPoint, CCoinsCacheEntry> { }

    public class CoinsViewCacheCursor : IEnumerator<KeyValuePair<COutPoint, CCoinsCacheEntry>>
    {
        private IEnumerator<KeyValuePair<COutPoint, CCoinsCacheEntry>> _enumerator;
        private bool _willErase;

        public CoinsViewCacheCursor(long usage, CCoinsMap map, bool willErase)
        {
            _enumerator = map.GetEnumerator();
            _willErase = willErase;
        }

        public KeyValuePair<COutPoint, CCoinsCacheEntry> Current => _enumerator.Current;

        object IEnumerator.Current => Current;

        public void Dispose()
        {
            _enumerator.Dispose();
        }

        public bool MoveNext()
        {
            return _enumerator.MoveNext();
        }

        public void Reset()
        {
            _enumerator.Reset();
        }

        public IEnumerator<KeyValuePair<COutPoint, CCoinsCacheEntry>> GetEnumerator()
        {
            return this;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }

    public class COutPoint
    {
        public uint256 Hash { get; set; }
        public uint N { get; set; }

        public COutPoint(uint256 hash, uint n)
        {
            Hash = hash;
            N = n;
        }
    }

    public class CTxOut
    {
        public long NValue { get; set; }
        public byte[] ScriptPubKey { get; set; }

        public CTxOut()
        {
            ScriptPubKey = Array.Empty<byte>();
        }

        public void Serialize(Stream s)
        {
            s.Write(BitConverter.GetBytes(NValue), 0, sizeof(long));
            s.Write(BitConverter.GetBytes(ScriptPubKey.Length), 0, sizeof(int));
            s.Write(ScriptPubKey, 0, ScriptPubKey.Length);
        }

        public void Unserialize(Stream s)
        {
            byte[] nValueBytes = new byte[sizeof(long)];
            s.Read(nValueBytes, 0, sizeof(long));
            NValue = BitConverter.ToInt64(nValueBytes, 0);

            byte[] scriptPubKeyLengthBytes = new byte[sizeof(int)];
            s.Read(scriptPubKeyLengthBytes, 0, sizeof(int));
            int scriptPubKeyLength = BitConverter.ToInt32(scriptPubKeyLengthBytes, 0);

            ScriptPubKey = new byte[scriptPubKeyLength];
            s.Read(ScriptPubKey, 0, scriptPubKeyLength);
        }

        public bool IsNull()
        {
            return ScriptPubKey.Length == 0;
        }

        public void SetNull()
        {
            ScriptPubKey = Array.Empty<byte>();
        }
    }

    public class CTransaction
    {
        public bool IsCoinBase { get; set; }
        public List<CTxIn> Vin { get; set; }
        public List<CTxOut> Vout { get; set; }

        public CTransaction()
        {
            Vin = new List<CTxIn>();
            Vout = new List<CTxOut>();
        }
    }

    public class CTxIn
    {
        public COutPoint Prevout { get; set; }

        public CTxIn(COutPoint prevout)
        {
            Prevout = prevout;
        }
    }

    public class uint256
    {
        private byte[] _data;

        public uint256()
        {
            _data = new byte[32];
        }

        public byte[] Data => _data;
    }
}
