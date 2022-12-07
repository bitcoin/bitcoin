/*
 * immer: immutable data structures for C++
 * Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
 *
 * This software is distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
 */

var immer = Module

var N = 1000

var suite = new Benchmark.Suite('push')
    .add('Immutable.List', function(){
        var v = new Immutable.List
        for (var x = 0; x < N; ++x)
            v = v.push(x)
    })
    .add('mori.vector', function(){
        var v = mori.vector()
        for (var x = 0; x < N; ++x)
            v = mori.conj.f2(v, x)
    })
    .add('mori.vector-Transient', function(){
        var v = mori.mutable.thaw(mori.vector())
        for (var x = 0; x < N; ++x)
            v = mori.mutable.conj.f2(v, x)
        return mori.mutable.freeze(v)
    })
    .add('immer.Vector', function(){
        var v = new immer.Vector
        for (var x = 0; x < N; ++x) {
            var v_ = v
            v = v.push(x)
            v_.delete()
        }
    })
    .add('immer.VectorInt', function(){
        var v = new immer.VectorInt
        for (var x = 0; x < N; ++x) {
            var v_ = v
            v = v.push(x)
            v_.delete()
        }
        v.delete()
    })
    .add('immer.VectorNumber', function(){
        var v = new immer.VectorNumber
        for (var x = 0; x < N; ++x) {
            var v_ = v
            v = v.push(x)
            v_.delete()
        }
        v.delete()
    })
    .add('immer.VectorInt-Native', function(){
        immer.rangeSlow_int(0, N).delete()
    })
    .add('immer.VectorInt-NativeTransient', function(){
        immer.range_int(0, N).delete()
    })
    .add('immer.VectorDouble-Native', function(){
        immer.rangeSlow_double(0, N).delete()
    })
    .add('immer.VectorDouble-NativeTransient', function(){
        immer.range_double(0, N).delete()
    })
    .on('cycle', function(event) {
        console.log(String(event.target));
    })
    .on('complete', function() {
        console.log('Fastest is ' + this.filter('fastest').map('name'));
    })
    .run({ 'async': true })
