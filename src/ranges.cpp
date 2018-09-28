// A C++ program for merging overlapping ranges
#include "ranges.h"
using namespace std;
// Compares two ranges according to their starting range index.
bool compareRange(const CRange &i1, const CRange &i2)
{
	return (i1.start < i2.start);
}
bool compareRangeReverse(const CRange &i1, const CRange &i2)
{
	return (i1.start > i2.start);
}

// The main function that takes a set of ranges, merges
// overlapping ranges and put the result in output
void mergeRanges(vector<CRange>& arr, vector<CRange>& output)
{
	if (arr.empty())
		return;

	// sort the ranges in increasing order of start index
	std::sort(arr.begin(), arr.end(), compareRange);

	// push the first range to stack
	output.push_back(arr[0]);

	CRange top;

	// Start from the next range and merge if necessary
	for (unsigned int i = 1; i < arr.size(); i++)
	{
		// get range from stack top
		top = output.back();

		// if current range is not overlapping with stack top,
		// push it to the stack
		if (top.end + 1 < arr[i].start)
			output.push_back(arr[i]);

		// Otherwise update the ending index of top if ending of current
		// range is more
		else if (top.end < arr[i].end)
		{
			top.end = arr[i].end;
			output.pop_back();
			output.push_back(top);
		}
	}
}


void subtractRanges(vector<CRange> &arr, vector<CRange> &deletions, vector<CRange> &output)
{
	// Test if the given set has at least one range
	if (arr.empty() || deletions.empty())
		return;

	// sort the ranges in increasing order of start index
	std::sort(arr.begin(), arr.end(), compareRange);

	// sort the deletions in decreasing order of start index
	std::sort(deletions.begin(), deletions.end(), compareRangeReverse);
	CRange deletion;
	// Start from the beginning of the main range array from which we'll
	// delete another array of ranges
	for (unsigned int i = 0; i < arr.size(); i++)
	{
		// find the first deletion range that comes on or after
		// the current range element in the main array
		deletion = deletions.back();
		while (arr[i].start > deletion.end && deletions.size() > 1) {
			deletions.pop_back();
			deletion = deletions.back();
		}

		// if the current ranges end is before the deletion start
		// or its start is after the deletion end, then it is unaffected
		// by the deletion and add it verbatim to the output
		if (arr[i].end < deletion.start || arr[i].start > deletion.end) {
			output.push_back(arr[i]);
			continue;
		}

		// if the current range is within the deletion range, then it should
		// be deleted so just skip it and move on to the next range
		if (arr[i].start >= deletion.start && arr[i].end <= deletion.end) {
			continue;
		}

		// if the current range's start is before the deletion range and
		// its end is within the deletion range, then set the current range
		// end to 1 before the deletion start and add it to the output
		if (arr[i].start < deletion.start && arr[i].end <= deletion.end) {
			arr[i].end = deletion.start - 1;
			output.push_back(arr[i]);
			continue;
		}

		// if the current range's end is after the deletion end and its
		// start is within the deletion, when set its start to be 1
		// after the deletion end, pop the deletion array, and reprocess
		// this same element again with the next deletion range
		if (arr[i].end > deletion.end && arr[i].start >= deletion.start) {
			arr[i].start = deletion.end + 1;
			i--;
			if (deletions.size() > 1) {
				deletions.pop_back();
			}
			continue;
		}

		// if the current range's end is after the deletion end and its
		// start is before the deletion, then add a range to the output
		// which starts at the current range start and ends right before
		// the deletion, and then set the current range start to be 1
		// after the deletion end, pop the deletion array, and reprocess
		// this same element again with the next deletion range    
		if (arr[i].start < deletion.start && arr[i].end > deletion.end) {
			CRange r(arr[i].start, deletion.start - 1);
			arr[i].start = deletion.end + 1;
			output.push_back(r);
			i--;
			if (deletions.size() > 1) {
				deletions.pop_back();
			}
			continue;
		}
	}
}
// validate and get count of range at same time. RangeCount > 0 is valid otherwise invalid
unsigned int validateRangesAndGetCount(const vector<CRange> &arr) {
	unsigned int total = 0;
	unsigned int nLastEnd = 0;
	for (auto& range : arr) {
		// ensure range is well formed
		if (range.end < range.start)
			return 0;
		if (range.start > 0 && range.start <= nLastEnd)
			return 0;
		total += (range.end - range.start) + 1;
		nLastEnd = range.end;
	}
	return total;
}
// does child ranges exist fully in the parent ranges
bool doesRangeContain(const vector<CRange> &parent, const vector<CRange> &child) {
	if (parent.empty() || child.empty())
		return false;
	// we just need to prove that a single child doesn't exist in parent range's to prove this false, otherwise it must be true
	for (auto& childRange : child) {
		bool found = false;
		for (auto& parentRange : parent) {
			if (childRange.start >= parentRange.start && childRange.end <= parentRange.end) {
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}
