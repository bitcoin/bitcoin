export async function generateFeedback({ result, risk, pr, files, commits, authorData, apiKey }) {
  if (!apiKey) return "_AI feedback unavailable — API key not set._";

  const isKimi = apiKey.startsWith("sk-") && !!process.env.KIMI_API_KEY;

  const endpoint = isKimi
    ? "https://api.moonshot.cn/v1/chat/completions"
    : "https://api.anthropic.com/v1/messages";

  const failedChecks = result.checks
    .filter(c => !c.passed)
    .map(c => `- ${c.name} (${c.score}/${c.max}): ${c.detail}`)
    .join("\n");

  const commitMessages = commits
    .slice(0, 10)
    .map(c => `- ${c.commit.message.split("\n")[0]}`)
    .join("\n");

  const changedFiles = files
    .slice(0, 15)
    .map(f => `- ${f.filename} (+${f.additions} -${f.deletions})`)
    .join("\n");

  const accountAgeDays = Math.floor(
    (Date.now() - new Date(authorData.created_at).getTime()) / (1000 * 60 * 60 * 24)
  );

  const prompt = `You are a Bitcoin Core contributor mentor. A contributor opened a pull request that failed the automated quality gate.

PR Title: ${pr.title}

PR Description:
${pr.body || "(no description provided)"}

Commit messages:
${commitMessages}

Changed files:
${changedFiles}

Risk level: ${risk.level} — ${risk.message.replace(/[*_`]/g, "")}

Failed checks:
${failedChecks}

Contributor: ${authorData.login}, account ${accountAgeDays} days old.

Write specific, actionable feedback referencing Bitcoin Core's CONTRIBUTING.md.

Rules:
- Be direct but respectful — assume good intent
- Be specific to Bitcoin Core conventions, not generic advice
- For missing prefix: name the exact prefix that fits based on the files changed
- For missing tests: name the exact test file and directory they should add to
- For bad commit messages: show a concrete rewritten example
- For large diffs: suggest which files to split into a separate PR
- For consensus changes without BIP: explain why a BIP is required
- Reference CONTRIBUTING.md: https://github.com/bitcoin/bitcoin/blob/master/CONTRIBUTING.md
- Under 300 words, markdown bullet points
- End with one encouraging sentence`;

  try {
    let responseText;

    if (isKimi) {
      const res = await fetch(endpoint, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          "Authorization": `Bearer ${apiKey}`,
        },
        body: JSON.stringify({
          model: "moonshot-v1-8k",
          temperature: 0.3,
          messages: [
            { role: "system", content: "You are a Bitcoin Core contributor mentor. Give specific, actionable feedback." },
            { role: "user",   content: prompt },
          ],
        }),
      });

      if (!res.ok) {
        console.error("Kimi API error:", await res.text());
        return "_AI feedback generation failed. Check KIMI_API_KEY secret._";
      }

      const data = await res.json();
      responseText = data.choices?.[0]?.message?.content;

    } else {
      const res = await fetch(endpoint, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          "x-api-key": apiKey,
          "anthropic-version": "2023-06-01",
        },
        body: JSON.stringify({
          model: "claude-sonnet-4-6",
          max_tokens: 1000,
          messages: [{ role: "user", content: prompt }],
        }),
      });

      if (!res.ok) {
        console.error("Anthropic API error:", await res.text());
        return "_AI feedback generation failed. Check ANTHROPIC_API_KEY secret._";
      }

      const data = await res.json();
      responseText = data.content?.[0]?.text;
    }

    return responseText || "_No feedback generated._";

  } catch (err) {
    console.error("Feedback error:", err);
    return "_AI feedback generation failed due to a network error._";
  }
}