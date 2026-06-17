const DEFAULT_ANTHROPIC_MODEL = "claude-3-5-sonnet-latest";

export async function generateFeedback({ result, pr, files, anthropicApiKey }) {
  const failedChecks = result.checks.filter((check) => !check.passed);

  if (failedChecks.length === 0) {
    return "### Feedback\n\nNo blocking quality-gate issues found. Maintainers can continue with normal code review.";
  }

  if (anthropicApiKey) {
    try {
      return await generateAiFeedback({ failedChecks, pr, files, anthropicApiKey });
    } catch (error) {
      console.log(`AI feedback failed, using fallback feedback: ${error.message}`);
    }
  }

  return generateFallbackFeedback(failedChecks, files);
}

async function generateAiFeedback({ failedChecks, pr, files, anthropicApiKey }) {
  const response = await fetch("https://api.anthropic.com/v1/messages", {
    method: "POST",
    headers: {
      "content-type": "application/json",
      "x-api-key": anthropicApiKey,
      "anthropic-version": "2023-06-01",
    },
    body: JSON.stringify({
      model: process.env.ANTHROPIC_MODEL || DEFAULT_ANTHROPIC_MODEL,
      max_tokens: 700,
      messages: [
        {
          role: "user",
          content: buildPrompt({ failedChecks, pr, files }),
        },
      ],
    }),
  });

  if (!response.ok) {
    throw new Error(`Anthropic API returned ${response.status}`);
  }

  const data = await response.json();
  const text = data.content
    ?.filter((part) => part.type === "text")
    .map((part) => part.text)
    .join("\n\n")
    .trim();

  if (!text) {
    throw new Error("Anthropic API returned no text.");
  }

  return `### Specific Feedback\n\n${text}`;
}

function buildPrompt({ failedChecks, pr, files }) {
  const changedFiles = files.map((file) => file.filename).slice(0, 30).join("\n");
  const failedDetails = failedChecks
    .map((check) => `- ${check.name}: ${check.detail}`)
    .join("\n");

  return `You are Core-Gate, a concise PR quality assistant for Bitcoin Core.

Write direct, actionable markdown feedback for the contributor.
Do not praise, do not be vague, and do not mention that you are an AI.
Focus only on the failed checks.

PR title: ${pr.title}
PR author: ${pr.user?.login ?? "unknown"}

Failed checks:
${failedDetails}

Changed files:
${changedFiles}`;
}

function generateFallbackFeedback(failedChecks, files) {
  const sections = failedChecks.map((check) => {
    switch (check.id) {
      case "title-prefix":
        return `**Component prefix:** Rename the PR title so it starts with the area being changed, for example \`wallet:\`, \`rpc:\`, \`test:\`, \`doc:\`, \`build:\`, or \`consensus:\`.`;
      case "description":
        return "**PR description:** Add a short explanation of the problem, why the change is needed, and how your approach fixes it. Aim for enough detail that a maintainer understands the reason before reading the diff.";
      case "commit-messages":
        return "**Commit messages:** Rewrite vague commit subjects like `fix`, `update`, or `wip`. Use a specific subject under 50 characters, such as `wallet: fix fee rounding`.";
      case "diff-size":
        return "**Diff size:** This PR is large enough that it may be hard to review. Split unrelated changes into separate PRs, or reduce the diff to one focused behavior change.";
      case "test-coverage":
        return buildTestFeedback(files);
      default:
        return `**${check.name}:** ${check.detail}`;
    }
  });

  return `### Specific Feedback\n\n${sections.join("\n\n")}`;
}

function buildTestFeedback(files) {
  const sourceFile = files.find((file) => isSourceFile(file.filename));

  if (!sourceFile) {
    return "**Tests:** Add or update a relevant test for the behavior changed by this PR.";
  }

  return `**Tests:** This PR changes \`${sourceFile.filename}\` without a matching test update. Add or update coverage in \`src/test/\`, \`test/\`, or \`src/test/fuzz/\` where appropriate.`;
}

function isSourceFile(filename) {
  if (
    filename.includes("/test/") ||
    filename.includes("/tests/") ||
    filename.includes("/fuzz/")
  ) {
    return false;
  }

  return filename.startsWith("src/") && /\.(c|cc|cpp|h|hpp)$/i.test(filename);
}
