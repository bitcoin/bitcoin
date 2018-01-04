// A C++ program for merging overlapping ranges
#include "ranges.h"

// Compares two ranges according to their starting range index.
bool compareRange(const CRange &i1, const CRange &i2)
{
	return (i1.start < i2.start);
}

// The main function that takes a set of ranges, merges
// overlapping ranges and prints the result
void mergeRanges(vector<CRange>& arr, vector<CRange>& output)
{
	if (arr.empty())
		return;

	// sort the ranges in increasing order of start index
	std::sort(arr.begin(), arr.end(), compareRange);

	// push the first range to stack
	output.push_back(arr[0]);

	// Start from the next range and merge if necessary
	for (int i = 1; i < arr.size(); i++)
	{
		// get range from stack top
		CRange top = output.front();

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


void subtractRanges(vector<CRange> &arr, vector<CRange> &del, vector<CRange> &output)
{
	// Test if the given set has at least one range
	if (arr.empty() || del.empty())
		return;

	// sort the ranges in increasing order of start index
	std::sort(arr.begin(), arr.end(), compareRange);

	// Create an empty stack of ranges
	vector<CRange> deletions;

	// sort the deletions in increasing order of start index
	sort(del.begin(), del.end(), compareRange);

	// add the deletions to the stack, the first deletion is at the top
	for (int i = del.size() - 1; i >= 0; i--) {
		deletions.push_back(del[i]);
	}

	// Start from the beginning of the main range array from which we'll
	// delete another array of ranges
	for (int i = 0; i < arr.size(); i++)
	{
		// find the first deletion range that comes on or after
		// the current range element in the main array
		CRange deletion = deletions.front();
		while (arr[i].start > deletion.end && deletions.size() > 1) {
			deletions.pop_back();
			deletion = deletions.front();
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
/*
int main()
{
	Range arr1[] = { { 3,3 },{ 5,6 },{ 4,4 },{ 12,13 },{ 8,10 } };
	int n1 = sizeof(arr1) / sizeof(arr1[0]);

	mergeRanges(arr1, n1);

	Range arr2[] = { { 2,10 },{ 12,15 } };
	int n2 = sizeof(arr2) / sizeof(arr2[0]);

	Range del[] = { { 5,5 },{ 7,9 },{ 14,14 } };
	int dn = sizeof(del) / sizeof(del[0]);

	subtractRanges(arr2, n2, del, dn);

	return 0;
}
*/
