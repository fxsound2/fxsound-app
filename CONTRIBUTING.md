## How to contribute
We welcome anyone who wants to contribute to this project. We thank you for all your contributions.

### Contribute as a Community
- You can answer questions in [the issue tracker](https://github.com/fxsound2/fxsound-app/issues), or in [the fxsound forum](https://forum.fxsound.com/).

### Contribute as a Developer
- Look for [issues labelled ’Good First Issue’](https://github.com/fxsound2/fxsound-app/issues?q=is%3Aopen+is%3Aissue+label%3A%22good+first+issue%22). These issues are for people who want to contribute, but try to work on a small feature first.
- Look for [issues labelled ‘Help Wanted’](https://github.com/fxsound2/fxsound-app/issues?q=is%3Aopen+is%3Aissue+label%3A%22help+wanted%22). These issues are relatively easy to solve.
- Look for [issues labelled ‘Bugs’](https://github.com/fxsound2/fxsound-app/issues?q=is%3Aopen+is%3Aissue+label%3Abug+) if you are looking to have an immediate impact.
- Look for [issues labeled 'Enhancement'](https://github.com/fxsound2/fxsound-app/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement). Solving these issues need a strong discussion, and it has the highest impact.
- Issues are tracked in [GitHub issues](https://github.com/fxsound2/fxsound-app/issues). To make contributions to the project create a branch from an issue and submit a Pull Request with your code changes.

## Branch Naming Convention

We follow best practices in branch naming.

### Regular Git Branches
Regular branches in Git are long-lived branches that are a stable and structured way to organize ongoing work. Those branches are permanently available in the repository, and their naming is straightforward.

Some regular git branches are:
- Master (`master/main`) branch. The default production branch in a Git repository that needs to be permanently stable. Developers can merge changes into the master branch only after code review and testing. All collaborators on a project must keep the master branch stable and updated.
- Development (`dev`) branch. The main development branch that serves as a central hub for developers to integrate new features, bug fixes, and other changes. Its primary purpose is to be the place for making changes to keep the developers from implementing them directly in the master branch. Developers test, review, and merge the changes from the dev into the master branch.
- Test (`test`) branch. The branch that contains all the code ready for QA and automation testing. QA testing is necessary before implementing any change in the production environment to maintain a stable codebase.

### Temporary Git Branches
Temporary branches are short-lived and disposable. Those branches serve specific, short-term purposes and are often deleted afterward.

Some of the temporary branches in Git are:

- Bugfix (`bugfix`) branch. The bugfix branch contains the code with bugs that require prompt fixes. It can be the rejected code from feature branches that needs fixing before implementation.
Hotfix branch. The hotfix branch is a place for implementing a temporary solution into a buggy code without adhering to the usual procedure. The hotfix branch is used in an emergency and when a fast fix is needed. Developers merge the hotfix branch directly into the production branch and later into the development branch.
- Feature (`feature`) branch. A feature branch serves to add, reconfigure, or remove a feature. The feature branch is based on the development branch. After the changes are made, developers merge the feature branch back into the development branch.
- Documentation (`docs`) branches: These branches are used to write, update, or fix documentation. Use the prefix `docs/`. For instance, `docs/api-endpoints`.
- Experimental (`experimental`) branch. This branch serves as a place for developing new features or ideas that are not part of a release or a sprint. It is a branch for trying out new things.
- WIP (`wip`) branch. Devs use WIP (work in progress) branches to develop or try out new features. These branches may not necessarily be part of the regular development workflow. WIP branches are project-specific and often informal, with no specific or standardized rules.
- Merging (`merging`) branch. A merging branch is a temporary branch used for resolving merge conflicts. Conflicts can arise when merging the latest development and a feature or hotfix branch into another branch. The merging branch is also useful when combining two branches of a feature developed by multiple contributors. This process involves merging, verifying, and finalizing the changes.

We follow [this guideline](https://tilburgsciencehub.com/building-blocks/collaborate-and-share-your-work/use-github/naming-git-branches/) for best practices in naming branches, and they are:
1. Use Separators
When writing a branch name, using separators such as hyphen (-) or slash (/) helps to increase readability of the name. But remember to be consistent with the chosen separator for all branches names.

    Example: `optimize-data-analysis or optimize/data/analysis`

2. Start Name with Category Word
It is recommended to begin the name of a branch with a category word, which indicates the type of task that is being solved with that branch. Some of the most used category words are:

    | Category Word | Meaning |
    |---|---|
    | hotfix | for quickly fixing critical issues, usually with a temporary solution |
    | bugfix | for fixing a bug |
    | feature | for adding, removing or modifying a feature |
    | test | for experimenting something which is not an issue |
    | wip | for a work in progress |

3. Avoid Using Numbers Only
It’s not a good practice to name a branch by only using numbers, because it creates confusion and increases chances of making mistakes. Instead, combine ID of issues with key words for the respective task.

4. Avoid Long Branch Names
As much as the branch name needs to be informative, it also needs to be precise and short. Detailed and long names can affect readability and efficiency.

5. Be Consistent
Consistency is key. After choosing one or more conventions, stick to them throughout the project.

## Conventional Commits

We follow [conventional commits syntax](https://www.conventionalcommits.org/) for our commits. It provides an easy set of rules for creating an explicit commit history, some of them are:
- **fix:** a commit of the type `fix` patches a bug in your codebase.
- **feat:** a commit of the type feat introduces a new feature to the codebase.
- **BREAKING CHANGE:** a commit that has a footer `BREAKING CHANGE:`, or appends a `!` after the type/scope, introduces a breaking API change.
- types other than `fix:` and `feat:` are allowed, for example [@commitlint/config-conventional](https://github.com/conventional-changelog/commitlint/tree/master/%40commitlint/config-conventional) (based on the Angular convention) recommends `build:`, `chore:`, `ci:`, `docs:`, `style:`, `refactor:`, `perf:`, `test:`, and others.
- footers other than `BREAKING CHANGE: <description>` may be provided and follow a convention similar to git trailer format. 

Every commit message needs to be written in lowercase.
- ✅ feat(lang): added new feature
- ❌ feat(lang): Added new feature

## Coding Style
Please follow the coding style as per the module (FxSound_App, audiopassthru, DfxDsp) that you are contributing to. FxSound_App which is developed in C++ follows [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).