(function ($, w, undefined) {
    if (w.footable === undefined || w.footable === null)
        throw new Error('Please check and make sure footable.js is included in the page and is loaded prior to this script.');

    var defaults = {
        sort: true,
        sorters: {
            alpha: function (a, b) {
                if (a === b) return 0;
                if (a < b) return -1;
                return 1;
            },
            numeric: function (a, b) {
                return a - b;
            }
        },
        classes: {
            sort: {
                sortable: 'footable-sortable',
                sorted: 'footable-sorted',
                descending: 'footable-sorted-desc',
                indicator: 'footable-sort-indicator'
            }
        },
        events: {
            sort: {
                sorting: 'footable_sorting',
                sorted: 'footable_sorted'
            }
        }
    };

    function Sort() {
        var p = this;
        p.name = 'Footable Sortable';
        p.init = function (ft) {
            p.footable = ft;
            if (ft.options.sort === true) {
                $(ft.table).bind({
                    'footable_initialized': function (e) {
                        var $table = $(ft.table),
                            $tbody = $table.find('> tbody'),
                            cls = ft.options.classes.sort,
                            column, $th;

                        if ($table.data('sort') === false) return;

                        $table.find('> thead > tr:last-child > th, > thead > tr:last-child > td').each(function (ec) {
                            $th = $(this), column = ft.columns[$th.index()];
                            if (column.sort.ignore !== true && !$th.hasClass(cls.sortable)) {
                                $th.addClass(cls.sortable);
                                $('<span />').addClass(cls.indicator).appendTo($th);
                            }
                        });

                        $table.find('> thead > tr:last-child > th.' + cls.sortable + ', > thead > tr:last-child > td.' + cls.sortable).unbind('click.footable').bind('click.footable', function (ec) {
                            ec.preventDefault();
                            $th = $(this);
                            var ascending = !$th.hasClass(cls.sorted);
                            p.doSort($th.index(), ascending);
                            return false;
                        });

                        var didSomeSorting = false;
                        for (var c in ft.columns) {
                            column = ft.columns[c];
                            if (column.sort.initial) {
                                var ascending = (column.sort.initial !== 'descending');
                                p.doSort(column.index, ascending);
                                break;
                            }
                        }
                        if (didSomeSorting) {
                            ft.bindToggleSelectors();
                        }
                    },
                    'footable_redrawn': function(e) {
                        var $table = $(ft.table),
                            cls = ft.options.classes.sort;
                        if ($table.data('sorted') >= 0) {
                            $table.find('> thead > tr:last-child > th').each(function(i){
                                var $th = $(this);
                                if ($th.hasClass(cls.sorted) || $th.hasClass(cls.descending)) {
                                    p.doSort(i);
                                    return;
                                }
                            });
                        }
                    },
                    'footable_column_data': function (e) {
                        var $th = $(e.column.th);
                        e.column.data.sort = e.column.data.sort || {};
                        e.column.data.sort.initial = $th.data('sort-initial') || false;
                        e.column.data.sort.ignore = $th.data('sort-ignore') || false;
                        e.column.data.sort.selector = $th.data('sort-selector') || null;

                        var match = $th.data('sort-match') || 0;
                        if (match >= e.column.data.matches.length) match = 0;
                        e.column.data.sort.match = e.column.data.matches[match];
                    }
                })
                //save the sort object onto the table so we can access it later
                .data('footable-sort', p);
            }
        };

        p.doSort = function(columnIndex, ascending) {
            var ft = p.footable;
            if ($(ft.table).data('sort') === false) return;

            var $table = $(ft.table),
                $tbody = $table.find('> tbody'),
                column = ft.columns[columnIndex],
                $th = $table.find('> thead > tr:last-child > th:eq(' + columnIndex + ')'),
                cls = ft.options.classes.sort,
                evt = ft.options.events.sort;

            ascending = (ascending === undefined) ? $th.hasClass(cls.sorted) :
                (ascending === 'toggle') ? !$th.hasClass(cls.sorted) : ascending;

            if (column.sort.ignore === true) return true;

            //raise a pre-sorting event so that we can cancel the sorting if needed
            var event = ft.raise(evt.sorting, { column: column, direction: ascending ? 'ASC' : 'DESC' });
            if (event && event.result === false) return;

            $table.data('sorted', column.index);

            $table.find('> thead > tr:last-child > th, > thead > tr:last-child > td').not($th).removeClass(cls.sorted + ' ' + cls.descending);

            if (ascending === undefined) {
                ascending = $th.hasClass(cls.sorted);
            }

            if (ascending) {
                $th.removeClass(cls.descending).addClass(cls.sorted);
            } else {
                $th.removeClass(cls.sorted).addClass(cls.descending);
            }

            p.sort(ft, $tbody, column, ascending);

            ft.bindToggleSelectors();
            ft.raise(evt.sorted, { column: column, direction: ascending ? 'ASC' : 'DESC' });
        };

        p.rows = function (ft, tbody, column) {
            var rows = [];
            tbody.find('> tr').each(function () {
                var $row = $(this), $next = null;
                if ($row.hasClass(ft.options.classes.detail)) return true;
                if ($row.next().hasClass(ft.options.classes.detail)) {
                    $next = $row.next().get(0);
                }
                var row = { 'row': $row, 'detail': $next };
                if (column !== undefined) {
                    row.value = ft.parse(this.cells[column.sort.match], column);
                }
                rows.push(row);
                return true;
            }).detach();
            return rows;
        };

        p.sort = function (ft, tbody, column, ascending) {
            var rows = p.rows(ft, tbody, column);
            var sorter = ft.options.sorters[column.type] || ft.options.sorters.alpha;
            rows.sort(function (a, b) {
                if (ascending) {
                    return sorter(a.value, b.value);
                } else {
                    return sorter(b.value, a.value);
                }
            });
            for (var j = 0; j < rows.length; j++) {
                tbody.append(rows[j].row);
                if (rows[j].detail !== null) {
                    tbody.append(rows[j].detail);
                }
            }
        };
    }

    w.footable.plugins.register(Sort, defaults);

})(jQuery, window);