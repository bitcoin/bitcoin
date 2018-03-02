# This code supports verifying group implementations which have branches
# or conditional statements (like cmovs), by allowing each execution path
# to independently set assumptions on input or intermediary variables.
#
# The general approach is:
# * A constraint is a tuple of two sets of of symbolic expressions:
#   the first of which are required to evaluate to zero, the second of which
#   are required to evaluate to nonzero.
#   - A constraint is said to be conflicting if any of its nonzero expressions
#     is in the ideal with basis the zero expressions (in other words: when the
#     zero expressions imply that one of the nonzero expressions are zero).
# * There is a list of laws that describe the intended behaviour, including
#   laws for addition and doubling. Each law is called with the symbolic point
#   coordinates as arguments, and returns:
#   - A constraint describing the assumptions under which it is applicable,
#     called "assumeLaw"
#   - A constraint describing the requirements of the law, called "require"
# * Implementations are transliterated into functions that operate as well on
#   algebraic input points, and are called once per combination of branches
#   exectured. Each execution returns:
#   - A constraint describing the assumptions this implementation requires
#     (such as Z1=1), called "assumeFormula"
#   - A constraint describing the assumptions this specific branch requires,
#     but which is by construction guaranteed to cover the entire space by
#     merging the results from all branches, called "assumeBranch"
#   - The result of the computation
# * All combinations of laws with implementation branches are tried, and:
#   - If the combination of assumeLaw, assumeFormula, and assumeBranch results
#     in a conflict, it means this law does not apply to this branch, and it is
#     skipped.
#   - For others, we try to prove the require constraints hold, assuming the
#     information in assumeLaw + assumeFormula + assumeBranch, and if this does
#     not succeed, we fail.
#     + To prove an expression is zero, we check whether it belongs to the
#       ideal with the assumed zero expressions as basis. This test is exact.
#     + To prove an expression is nonzero, we check whether each of its
#       factors is contained in the set of nonzero assumptions' factors.
#       This test is not exact, so various combinations of original and
#       reduced expressions' factors are tried.
#   - If we succeed, we print out the assumptions from assumeFormula that
#     weren't implied by assumeLaw already. Those from assumeBranch are skipped,
#     as we assume that all constraints in it are complementary with each other.
#
# Based on the sage verification scripts used in the Explicit-Formulas Database
# by Tanja Lange and others, see http://hyperelliptic.org/EFD

class fastfrac:
  """Fractions over rings."""

  def __init__(self,R,top,bot=1):
    """Construct a fractional, given a ring, a numerator, and denominator."""
    self.R = R
    if parent(top) == ZZ or parent(top) == R:
      self.top = R(top)
      self.bot = R(bot)
    elif top.__class__ == fastfrac:
      self.top = top.top
      self.bot = top.bot * bot
    else:
      self.top = R(numerator(top))
      self.bot = R(denominator(top)) * bot

  def iszero(self,I):
    """Return whether this fraction is zero given an ideal."""
    return self.top in I and self.bot not in I

  def reduce(self,assumeZero):
    zero = self.R.ideal(map(numerator, assumeZero))
    return fastfrac(self.R, zero.reduce(self.top)) / fastfrac(self.R, zero.reduce(self.bot))

  def __add__(self,other):
    """Add two fractions."""
    if parent(other) == ZZ:
      return fastfrac(self.R,self.top + self.bot * other,self.bot)
    if other.__class__ == fastfrac:
      return fastfrac(self.R,self.top * other.bot + self.bot * other.top,self.bot * other.bot)
    return NotImplemented

  def __sub__(self,other):
    """Subtract two fractions."""
    if parent(other) == ZZ:
      return fastfrac(self.R,self.top - self.bot * other,self.bot)
    if other.__class__ == fastfrac:
      return fastfrac(self.R,self.top * other.bot - self.bot * other.top,self.bot * other.bot)
    return NotImplemented

  def __neg__(self):
    """Return the negation of a fraction."""
    return fastfrac(self.R,-self.top,self.bot)

  def __mul__(self,other):
    """Multiply two fractions."""
    if parent(other) == ZZ:
      return fastfrac(self.R,self.top * other,self.bot)
    if other.__class__ == fastfrac:
      return fastfrac(self.R,self.top * other.top,self.bot * other.bot)
    return NotImplemented

  def __rmul__(self,other):
    """Multiply something else with a fraction."""
    return self.__mul__(other)

  def __div__(self,other):
    """Divide two fractions."""
    if parent(other) == ZZ:
      return fastfrac(self.R,self.top,self.bot * other)
    if other.__class__ == fastfrac:
      return fastfrac(self.R,self.top * other.bot,self.bot * other.top)
    return NotImplemented

  def __pow__(self,other):
    """Compute a power of a fraction."""
    if parent(other) == ZZ:
      if other < 0:
        # Negative powers require flipping top and bottom
        return fastfrac(self.R,self.bot ^ (-other),self.top ^ (-other))
      else:
        return fastfrac(self.R,self.top ^ other,self.bot ^ other)
    return NotImplemented

  def __str__(self):
    return "fastfrac((" + str(self.top) + ") / (" + str(self.bot) + "))"
  def __repr__(self):
    return "%s" % self

  def numerator(self):
    return self.top

class constraints:
  """A set of constraints, consisting of zero and nonzero expressions.

  Constraints can either be used to express knowledge or a requirement.

  Both the fields zero and nonzero are maps from expressions to description
  strings. The expressions that are the keys in zero are required to be zero,
  and the expressions that are the keys in nonzero are required to be nonzero.

  Note that (a != 0) and (b != 0) is the same as (a*b != 0), so all keys in
  nonzero could be multiplied into a single key. This is often much less
  efficient to work with though, so we keep them separate inside the
  constraints. This allows higher-level code to do fast checks on the individual
  nonzero elements, or combine them if needed for stronger checks.

  We can't multiply the different zero elements, as it would suffice for one of
  the factors to be zero, instead of all of them. Instead, the zero elements are
  typically combined into an ideal first.
  """

  def __init__(self, **kwargs):
    if 'zero' in kwargs:
      self.zero = dict(kwargs['zero'])
    else:
      self.zero = dict()
    if 'nonzero' in kwargs:
      self.nonzero = dict(kwargs['nonzero'])
    else:
      self.nonzero = dict()

  def negate(self):
    return constraints(zero=self.nonzero, nonzero=self.zero)

  def __add__(self, other):
    zero = self.zero.copy()
    zero.update(other.zero)
    nonzero = self.nonzero.copy()
    nonzero.update(other.nonzero)
    return constraints(zero=zero, nonzero=nonzero)

  def __str__(self):
    return "constraints(zero=%s,nonzero=%s)" % (self.zero, self.nonzero)

  def __repr__(self):
    return "%s" % self


def conflicts(R, con):
  """Check whether any of the passed non-zero assumptions is implied by the zero assumptions"""
  zero = R.ideal(map(numerator, con.zero))
  if 1 in zero:
    return True
  # First a cheap check whether any of the individual nonzero terms conflict on
  # their own.
  for nonzero in con.nonzero:
    if nonzero.iszero(zero):
      return True
  # It can be the case that entries in the nonzero set do not individually
  # conflict with the zero set, but their combination does. For example, knowing
  # that either x or y is zero is equivalent to having x*y in the zero set.
  # Having x or y individually in the nonzero set is not a conflict, but both
  # simultaneously is, so that is the right thing to check for.
  if reduce(lambda a,b: a * b, con.nonzero, fastfrac(R, 1)).iszero(zero):
    return True
  return False


def get_nonzero_set(R, assume):
  """Calculate a simple set of nonzero expressions"""
  zero = R.ideal(map(numerator, assume.zero))
  nonzero = set()
  for nz in map(numerator, assume.nonzero):
    for (f,n) in nz.factor():
      nonzero.add(f)
    rnz = zero.reduce(nz)
    for (f,n) in rnz.factor():
      nonzero.add(f)
  return nonzero


def prove_nonzero(R, exprs, assume):
  """Check whether an expression is provably nonzero, given assumptions"""
  zero = R.ideal(map(numerator, assume.zero))
  nonzero = get_nonzero_set(R, assume)
  expl = set()
  ok = True
  for expr in exprs:
    if numerator(expr) in zero:
      return (False, [exprs[expr]])
  allexprs = reduce(lambda a,b: numerator(a)*numerator(b), exprs, 1)
  for (f, n) in allexprs.factor():
    if f not in nonzero:
      ok = False
  if ok:
    return (True, None)
  ok = True
  for (f, n) in zero.reduce(numerator(allexprs)).factor():
    if f not in nonzero:
      ok = False
  if ok:
    return (True, None)
  ok = True
  for expr in exprs:
    for (f,n) in numerator(expr).factor():
      if f not in nonzero:
        ok = False
  if ok:
    return (True, None)
  ok = True
  for expr in exprs:
    for (f,n) in zero.reduce(numerator(expr)).factor():
      if f not in nonzero:
        expl.add(exprs[expr])
  if expl:
    return (False, list(expl))
  else:
    return (True, None)


def prove_zero(R, exprs, assume):
  """Check whether all of the passed expressions are provably zero, given assumptions"""
  r, e = prove_nonzero(R, dict(map(lambda x: (fastfrac(R, x.bot, 1), exprs[x]), exprs)), assume)
  if not r:
    return (False, map(lambda x: "Possibly zero denominator: %s" % x, e))
  zero = R.ideal(map(numerator, assume.zero))
  nonzero = prod(x for x in assume.nonzero)
  expl = []
  for expr in exprs:
    if not expr.iszero(zero):
      expl.append(exprs[expr])
  if not expl:
    return (True, None)
  return (False, expl)


def describe_extra(R, assume, assumeExtra):
  """Describe what assumptions are added, given existing assumptions"""
  zerox = assume.zero.copy()
  zerox.update(assumeExtra.zero)
  zero = R.ideal(map(numerator, assume.zero))
  zeroextra = R.ideal(map(numerator, zerox))
  nonzero = get_nonzero_set(R, assume)
  ret = set()
  # Iterate over the extra zero expressions
  for base in assumeExtra.zero:
    if base not in zero:
      add = []
      for (f, n) in numerator(base).factor():
        if f not in nonzero:
          add += ["%s" % f]
      if add:
        ret.add((" * ".join(add)) + " = 0 [%s]" % assumeExtra.zero[base])
  # Iterate over the extra nonzero expressions
  for nz in assumeExtra.nonzero:
    nzr = zeroextra.reduce(numerator(nz))
    if nzr not in zeroextra:
      for (f,n) in nzr.factor():
        if zeroextra.reduce(f) not in nonzero:
          ret.add("%s != 0" % zeroextra.reduce(f))
  return ", ".join(x for x in ret)


def check_symbolic(R, assumeLaw, assumeAssert, assumeBranch, require):
  """Check a set of zero and nonzero requirements, given a set of zero and nonzero assumptions"""
  assume = assumeLaw + assumeAssert + assumeBranch

  if conflicts(R, assume):
    # This formula does not apply
    return None

  describe = describe_extra(R, assumeLaw + assumeBranch, assumeAssert)

  ok, msg = prove_zero(R, require.zero, assume)
  if not ok:
    return "FAIL, %s fails (assuming %s)" % (str(msg), describe)

  res, expl = prove_nonzero(R, require.nonzero, assume)
  if not res:
    return "FAIL, %s fails (assuming %s)" % (str(expl), describe)

  if describe != "":
    return "OK (assuming %s)" % describe
  else:
    return "OK"


def concrete_verify(c):
  for k in c.zero:
    if k != 0:
      return (False, c.zero[k])
  for k in c.nonzero:
    if k == 0:
      return (False, c.nonzero[k])
  return (True, None)
