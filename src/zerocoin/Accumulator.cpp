/**
 * @file       Accumulator.cpp
 *
 * @brief      Accumulator and AccumulatorWitness classes for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/

#include <sstream>
#include "Zerocoin.h"

namespace libzerocoin {

//Accumulator class
Accumulator::Accumulator(const AccumulatorAndProofParams* p, const CoinDenomination d): params(p), denomination(d) {
	if (!(params->initialized)) {
		throw ZerocoinException("Invalid parameters for accumulator");
	}

	this->value = this->params->accumulatorBase;
}

Accumulator::Accumulator(const Params* p, const CoinDenomination d) {
	this->params = &(p->accumulatorParams);
	this->denomination = d;

	if (!(params->initialized)) {
		throw ZerocoinException("Invalid parameters for accumulator");
	}

	this->value = this->params->accumulatorBase;
}

void Accumulator::accumulate(const PublicCoin& coin) {
	// Make sure we're initialized
	if(!(this->value)) {
		throw ZerocoinException("Accumulator is not initialized");
	}

	if(this->denomination != coin.getDenomination()) {
		//std::stringstream msg;
		std::string msg;
		msg = "Wrong denomination for coin. Expected coins of denomination: ";
		msg += this->denomination;
		msg += ". Instead, got a coin of denomination: ";
		msg += coin.getDenomination();
		throw std::invalid_argument(msg);
	}

	if(coin.validate()) {
		// Compute new accumulator = "old accumulator"^{element} mod N
		this->value = this->value.pow_mod(coin.getValue(), this->params->accumulatorModulus);
	} else {
		throw std::invalid_argument("Coin is not valid");
	}
}

const CoinDenomination Accumulator::getDenomination() const {
	return static_cast<CoinDenomination> (this->denomination);
}

const Bignum& Accumulator::getValue() const {
	return this->value;
}

Accumulator& Accumulator::operator += (const PublicCoin& c) {
	this->accumulate(c);
	return *this;
}

bool Accumulator::operator == (const Accumulator rhs) const {
	return this->value == rhs.value;
}

//AccumulatorWitness class
AccumulatorWitness::AccumulatorWitness(const Params* p,
                                       const Accumulator& checkpoint, const PublicCoin coin): params(p), witness(checkpoint), element(coin) {
}

void AccumulatorWitness::AddElement(const PublicCoin& c) {
	if(element != c) {
		witness += c;
	}
}

const Bignum& AccumulatorWitness::getValue() const {
	return this->witness.getValue();
}

bool AccumulatorWitness::VerifyWitness(const Accumulator& a, const PublicCoin &publicCoin) const {
	Accumulator temp(witness);
	temp += element;
	return (temp == a && this->element == publicCoin);
}

AccumulatorWitness& AccumulatorWitness::operator +=(
    const PublicCoin& rhs) {
	this->AddElement(rhs);
	return *this;
}

} /* namespace libzerocoin */
