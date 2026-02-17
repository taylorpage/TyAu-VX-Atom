# Contributing to TyAu-Distortion

Thank you for contributing to TyAu-Distortion! This document outlines our commit message standards and contribution guidelines.

## Git Commit Message Standards

We follow the [Conventional Commits](https://www.conventionalcommits.org/) specification for all commit messages. This helps maintain a clear and consistent project history.

### Commit Message Format

```
<type>(<scope>): <subject>

[optional body]

[optional footer(s)]
```

### Type

The type must be one of the following:

- **feat**: A new feature
- **fix**: A bug fix
- **docs**: Documentation only changes
- **style**: Changes that do not affect the meaning of the code (white-space, formatting, missing semi-colons, etc)
- **refactor**: A code change that neither fixes a bug nor adds a feature
- **perf**: A code change that improves performance
- **test**: Adding missing tests or correcting existing tests
- **build**: Changes that affect the build system or external dependencies
- **ci**: Changes to our CI configuration files and scripts
- **chore**: Other changes that don't modify src or test files
- **revert**: Reverts a previous commit

### Scope

The scope is optional and should specify the place of the commit change. Examples for TyAu-Distortion:

- `ui`
- `dsp`
- `audio-engine`
- `parameters`
- `presets`

### Subject

The subject contains a succinct description of the change:

- Use the imperative, present tense: "change" not "changed" nor "changes"
- Don't capitalize the first letter
- No period (.) at the end
- Maximum 50 characters

### Body

The body should include the motivation for the change and contrast this with previous behavior:

- Use the imperative, present tense: "change" not "changed" nor "changes"
- Wrap at 72 characters
- Explain the "why" of the change, not the "what" (the diff shows what changed)

### Footer

The footer should contain any information about Breaking Changes and is also the place to reference GitHub issues that this commit closes.

Breaking Changes should start with the word `BREAKING CHANGE:` with a space or two newlines. The rest of the commit message is then used for this.

### Examples

#### Simple commit
```
feat(dsp): add soft clipping algorithm

Implement soft clipping to provide smoother distortion character.
This allows for more musical-sounding saturation.

Closes #42
```

#### Bug fix
```
fix(ui): correct knob parameter mapping

The drive knob was not properly scaling values, causing incorrect
gain staging. Fixed the logarithmic mapping calculation.

Fixes #123
```

#### Breaking change
```
feat(audio-engine)!: change buffer size to 512 samples

BREAKING CHANGE: The default buffer size has been changed from 256
to 512 samples. This improves performance on some systems but may
require configuration updates for low-latency setups.
```

#### Documentation
```
docs: update README with installation instructions

Add detailed steps for building the plugin on macOS and Windows,
including required dependencies and troubleshooting tips.
```

## Using the Commit Template

A commit template has been configured for this repository. When you run `git commit`, your editor will open with the template pre-populated with helpful reminders.

To manually set up the template:
```bash
git config commit.template .gitmessage
```

## Additional Guidelines

1. **Commit Often**: Make small, focused commits that represent a single logical change
2. **Test Before Committing**: Ensure your code builds and tests pass
3. **Review Your Changes**: Use `git diff --staged` before committing
4. **Write Descriptive Messages**: Future you (and other contributors) will thank you

## Questions?

If you have questions about these guidelines, please open an issue for discussion.
