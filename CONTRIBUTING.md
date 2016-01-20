# Contributing to Bitcoin Classic

Bitcoin Classic is hard forking bitcoin to a 2 MB blocksize limit. Please come and join us.

 - [Contribution Guidelines](#contribution-guidelines)
 - [Communication](#communication)
 - [Merge Window](#merge-window)
 - [Consider.it](#considerit)
 - [Code of Conduct](#codeofconduct)

## <a name="contribution-guidelines"></a> Contribution Guidelines

**Any change to Bitcoin must be respected by 3 groups:**

- Developers: which implement changes
- Miners: which give them computation
- Users: which give them value

**We accept pull requests using this rubric:**

- All pull requests must be within the [merge window](#merge-window).
- If the pull request has a majority on [consider.it](https://bitcoinclassic.consider.it/). All pull requests must have a vote. A proposal that does not have majority support on [consider.it](#considerit) can be closed at any time by the maintainers.
- If both Miners and Users want a feature, then we will accept the best pull request for that feature that Developers have implemented
- If both Miners and Users reject a feature, we will reject pull requests for it.
- A proposal is in conflict if: (1) Miners and Users disagree on it, or (2) another approved proposal disagrees with it. Github committers can break a tie until Miners and Users resolve their conflicts.
- If two or more voter-approved pull requests conflict either in code or on concept, then a run-off vote will be held comparing the conflicts.
- If either miners or users reject a pull request, it cannot be merged until such rejection is resolved.
- A proposal is in limbo if no Miners or Users have given an opinion. Github committers are free to act on any pull request in limbo, but may have to change their action if the proposal receives the missing votes.
- Any pull request can be removed for violation of these guidelines.

Github committers do the actual merging of pull requests and create the official releases. If voters change their mind, committers will un-merge the pull request or reopen a pull request. Please remember to read through pull requests before submitting your own to avoid conflicting requests. 

Issues are to be submitted [here](https://github.com/bitcoinclassic/bitcoinclassic/issues).

## <a name="communication"></a> Communication
Bitcoin Classic is designed to allow users, miners, and developers to express their opinions equally and transparently. To do so, multiple channels of communication have been set up, see below.

- [Slack Channel](http://invite.bitcoinclassic.com/)
- [Subreddit](https://www.reddit.com/r/bitcoin_classic)
- [Consider.it](https://bitcoinclassic.consider.it/)

## <a name="merge-window"></a> Merge Window
Using merge windows allows Bitcoin Classic to remain focused on specific issues at hand without distraction. A merge window decides the focus of development for the continuation of that window. In this window, code which is deemed to be sufficiently stable (and which is accepted by the community) is merged. Merge windows will be closed based on a vote via [consider.it](https://bitcoinclassic.consider.it/) and another vote will be held to open the next merge window. For details on the current merge window see below.

 - `Hard fork bitcoin to a 2 MB blocksize limit.`
 - `2 MB Test Net`
 - `Consensus Rule changes - O(n^2) protections`
 - `Activication / versionbits code`

Emergencies and bug fixes are outside the scope of merge windows.

## <a name="considerit"></a> Consider.it
By voting on issues, Bitcoin Classic aims to create a collective deliberation among all those using this implementation. Every vote must be respected by 3 groups: users, miners, and developers. **The votes are interpreted as such:**

- Each vote goes from -1 to 1, with 0 at neutral/undecided
- In the Miner vote, each Miner's vote is multiplied by their hashpower
- Only verified users have votes.

As voting is a continuous process, all voters are encouraged to vote for a pull request only if they believe it has been thoroughly tested, reviewed, and fitting of any other criteria the voter holds fit. If any vote is found to be subject of vote manipulation the vote will be considered void.

## <a name="codeofconduct"></a> Code of Conduct

We want a productive, happy and agile community that can welcome new ideas in a complex field, improve every process every year, and foster collaboration between groups with very different needs, interests and skills. We gain strength from diversity, and actively seek participation from those who enhance it. This code of conduct exists to ensure that diverse groups collaborate to mutual advantage and enjoyment. We will challenge prejudice that could jeopardise the participation of any person in the project.

The Code of Conduct governs how we behave in public or in private whenever the project will be judged by our actions. We expect it to be honoured by everyone who represents the project officially or informally, claims affiliation with the project, or participates directly.

**We strive to:**

- Be considerate
	- Our work will be used by other people, and we, in turn, will depend on the work of others. Any decision we take will affect users and colleagues, and we should consider them when making decisions.

- Be respectful
	- Disagreement is no excuse for poor manners. We work together to resolve conflict, assume good intentions and do our best to act in an empathic fashion. We don't allow frustration to turn into a personal attack. A community where people feel uncomfortable or threatened is not a productive one.

- Take responsibility for our words and our actions
	- We can all make mistakes; when we do, we take responsibility for them. If someone has been harmed or offended, we listen carefully and respectfully, and work to right the wrong.

- Be collaborative
	- What we produce is a complex whole made of many parts, it is the sum of many dreams. Collaboration between teams that each have their own goal and vision is essential; for the whole to be more than the sum of its parts, each part must make an effort to understand the whole.
	Collaboration reduces redundancy and improves the quality of our work. Internally and externally, we celebrate good collaboration. Wherever possible, we work closely with upstream projects and others in the Bitcoin community to coordinate our efforts. We prefer to work transparently and involve interested parties as early as possible.

- Value decisiveness, clarity and consensus
	- Disagreements, social and technical, are normal, but we do not allow them to persist and fester leaving others uncertain of the agreed direction.
	We expect participants in the project to resolve disagreements constructively. When they cannot, we escalate the matter to structures with designated leaders to arbitrate and provide clarity and direction.

- Ask for help when unsure
	- Nobody is expected to be perfect in this community. Asking questions early avoids many problems later, so questions are encouraged, though they may be directed to the appropriate forum. Those who are asked should be responsive and helpful.

- Step down considerately
	- When somebody leaves or disengages from the project, we ask that they do so in a way that minimises disruption to the project. They should tell people they are leaving and take the proper steps to ensure that others can pick up where they left off.

**Conflicts of interest**

We expect leaders to be aware when they are in conflict due to employment or other projects they are involved in and abstain or delegate decisions that may be seen to be self-interested. We expect that everyone who participates in the project does so with the goal of making life better for its users. When in doubt, ask for a second opinion. Perceived conflicts of interest are important to address; as a leader, act to ensure that decisions are credible even if they must occasionally be unpopular, difficult or favourable to the interests of one group over another.

This Code is not exhaustive or complete. It is not a rulebook; it serves to distil our common understanding of a collaborative, shared environment and goals. We expect it to be followed in spirit as much as in the letter.
