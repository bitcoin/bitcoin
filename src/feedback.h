#ifndef FEEDBACK_H
#define FEEDBACK_H
#include "script/script.h"
#include "serialize.h"
enum FeedbackUser {
	FEEDBACKNONE=0,
    FEEDBACKBUYER=1,
	FEEDBACKSELLER=2,
	FEEDBACKARBITER=3
};
class CNameTXIDTuple {
public:
	std::vector<unsigned char> first;
	uint256 second;

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(first);
		READWRITE(second);
	}

	CNameTXIDTuple(const std::vector<unsigned char> &f, uint256 s) {
		first = f;
		second = s;
	}

	CNameTXIDTuple() {
		SetNull();
	}
	inline CNameTXIDTuple operator=(const CNameTXIDTuple& other) {
		this->first = other.first;
		this->second = other.second;
		return *this;
	}
	inline bool operator==(const CNameTXIDTuple& other) const {
		return this->first == other.first && this->second == other.second;
	}
	inline bool operator!=(const CNameTXIDTuple& other) const {
		return (this->first != other.first || this->second != other.second);
	}
	void SetNull() {
		second.SetNull();
		first.clear();
	}

};
class CFeedback {
public:
	std::vector<unsigned char> vchFeedback;
	unsigned char nRating;
	unsigned char nFeedbackUserTo;
	unsigned char nFeedbackUserFrom;
    CFeedback() {
        SetNull();
    }
    CFeedback(unsigned char nAcceptFeedbackUserFrom, unsigned char nAcceptFeedbackUserTo) {
        SetNull();
		nFeedbackUserFrom = nAcceptFeedbackUserFrom;
		nFeedbackUserTo = nAcceptFeedbackUserTo;
    }
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		READWRITE(vchFeedback);
		READWRITE(VARINT(nRating));
		READWRITE(VARINT(nFeedbackUserFrom));
		READWRITE(VARINT(nFeedbackUserTo));
	}

    friend bool operator==(const CFeedback &a, const CFeedback &b) {
        return (
        a.vchFeedback == b.vchFeedback
		&& a.nRating == b.nRating
		&& a.nFeedbackUserFrom == b.nFeedbackUserFrom
		&& a.nFeedbackUserTo == b.nFeedbackUserTo
        );
    }

    CFeedback operator=(const CFeedback &b) {
        vchFeedback = b.vchFeedback;
		nRating = b.nRating;
		nFeedbackUserFrom = b.nFeedbackUserFrom;
		nFeedbackUserTo = b.nFeedbackUserTo;
        return *this;
    }

    friend bool operator!=(const CFeedback &a, const CFeedback &b) {
        return !(a == b);
    }

    void SetNull() {  nRating = 0; nFeedbackUserFrom = 0; nFeedbackUserTo = 0; vchFeedback.clear();}
    bool IsNull() const { return (  nRating == 0 && nFeedbackUserFrom == 0 && nFeedbackUserTo == 0 && vchFeedback.empty()); }
};
#endif // FEEDBACK_H