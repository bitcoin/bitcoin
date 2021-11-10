import gdb.printing

class OutcomeBasicOutcomePrinter(object):
    """Print an outcome::basic_outcome<T>"""

    def __init__(self, val):
        self.val = val

    def children(self):
        if self.val['_state']['_status']['status_value'] & 1 == 1:
            yield ('value', self.val['_state']['_value'])
        if self.val['_state']['_status']['status_value'] & 2 == 2:
            yield ('error', self.val['_state']['_error'])
        if self.val['_state']['_status']['status_value'] & 4 == 4:
            yield ('exception', self.val['_ptr'])

    def display_hint(self):
        return None

    def to_string(self):
        if self.val['_state']['_status']['status_value'] & 54 == 54:
            return 'errored (errno, moved from) + exceptioned'
        if self.val['_state']['_status']['status_value'] & 50 == 50:
            return 'errored (errno, moved from)'
        if self.val['_state']['_status']['status_value'] & 38 == 38:
            return 'errored + exceptioned (moved from)'
        if self.val['_state']['_status']['status_value'] & 36 == 36:
            return 'exceptioned (moved from)'
        if self.val['_state']['_status']['status_value'] & 35 == 35:
            return 'errored (moved from)'
        if self.val['_state']['_status']['status_value'] & 33 == 33:
            return 'valued (moved from)'
        if self.val['_state']['_status']['status_value'] & 22 == 22:
            return 'errored (errno) + exceptioned'
        if self.val['_state']['_status']['status_value'] & 18 == 18:
            return 'errored (errno)'
        if self.val['_state']['_status']['status_value'] & 6 == 6:
            return 'errored + exceptioned'
        if self.val['_state']['_status']['status_value'] & 4 == 4:
            return 'exceptioned'
        if self.val['_state']['_status']['status_value'] & 2 == 2:
            return 'errored'
        if self.val['_state']['_status']['status_value'] & 1 == 1:
            return 'valued'
        if self.val['_state']['_status']['status_value'] & 0xff == 0:
            return 'empty'


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("outcome_v2")
    pp.add_printer('outcome_v2::basic_result', '^outcome_v2[_0-9a-f]*::basic_result<.*>$', OutcomeBasicOutcomePrinter)
    pp.add_printer('outcome_v2::basic_outcome', '^outcome_v2[_0-9a-f]*::basic_outcome<.*>$', OutcomeBasicOutcomePrinter)
    return pp

def register_printers(obj = None):
    gdb.printing.register_pretty_printer(obj, build_pretty_printer(), replace = True)

register_printers(gdb.current_objfile())
