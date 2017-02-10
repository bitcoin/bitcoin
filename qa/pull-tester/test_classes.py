# Copyright (c) 2016-2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
RpcTest class and help functions which create Disabled/Skipped tests

>>> sometest=RpcTest("name")
>>> repr(sometest)
'name.py'
>>> sometest.is_disabled()
False
>>> sometest.reason is None
True
>>> sometest.skip_platforms
[]
>>> disabled_test=Disabled("foo", "foo does not work right now")
>>> disabled_test.is_disabled()
True
>>> disabled_test.reason
'foo does not work right now'
>>> disabled_test.skip_platforms
[]
>>> skipped_test=Skip("bar", "", "bar is skipped on all platforms because all matches empty string")
>>> skipped_test.is_skipped()
True
>>> unskipped_test=Skip("baz", "NoSuchPlatform", "baz is not skipped since there is no such platform")
>>> unskipped_test.is_skipped()
False
>>> testwithargs=RpcTest("name --somearg --anotherarg")
>>> repr(testwithargs)
'name.py --somearg --anotherarg'
"""

import platform


# collect as much platform info as we want people to be able to filter
# against for skipped tests
# platform.node() is not included because too easy to get an accidental match
# against a computer's name.
_PLATFORM_INFO =  [ platform.machine(),
                    platform.platform(),
                    platform.platform(aliased=1),
                    platform.platform(terse=1),
                    platform.system(),
                    ','.join(platform.architecture()),
                    ','.join(platform.uname()),
                  ]


class RpcTest(object):
    ''' Convenience class for RCP tests and disabled/skipped tests '''
    def __init__(self, obj):
        if isinstance(obj, RpcTest):
            self.name = obj.name
            self.args = obj.args
            self.disabled = obj.disabled
            self.reason = obj.reason
            self.skip_platforms = obj.skip_platforms
        else:
            words = str(obj).split(" ")  # need to split args
            self.name = words[0]
            if len(words) > 1:
                self.args = words[1:]
            else:
                self.args = []
            self.disabled = False
            self.reason = None
            self.skip_platforms = []

    def is_disabled(self):
        ''' returns True if test is explicitly disabled (completely) '''
        return self.disabled

    def is_skipped(self):
        ''' returns True if test would skip on this platform '''
        skip = False
        for platform_to_skip in self.skip_platforms:
            for pi in _PLATFORM_INFO:
                if platform_to_skip.lower() in pi.lower().split(','):
                    skip = True
                    break;
        return skip

    def disable(self, reason):
        ''' set test to explicitly disabled (completely) '''
        self.disabled = True
        self.reason = reason

    def skip(self, platforms, reason):
        ''' set test to skip on certain platforms '''
        self.reason = reason
        if isinstance(platforms, tuple) or isinstance(platforms, list):
            for i in platforms:
                self.skip_platforms.append(i)
        else:
                self.skip_platforms.append(platforms)

    def __repr__(self):
        retval = self.name + '.py'
        if self.args:
            retval = " ".join([retval, ] + self.args)
        return retval

    def __str__(self):
        return repr(self)


def Disabled(test_name, reason):
    ''' create a disabled test '''
    assert reason, "disabling a test requires a reason!"
    rpctest = RpcTest(test_name)
    rpctest.disable(reason)
    return rpctest


def Skip(test_name, platforms, reason):
    ''' create a test which is skipped on certain platforms.
        The 'platforms' parameter can be a string or tuple/list of strings
        which are matched against platform identifiers obtained when
        the test is executed.
    '''
    assert reason, "skipping a test on some platforms requires a reason!"
    rpctest = RpcTest(test_name)
    rpctest.skip(platforms, reason)
    return rpctest


if __name__ == "__main__":
    import doctest
    doctest.testmod()
