# Priv Helper document

## install 
If you haven't already installed GitFlow, you can do so using your package manager:
for Debian/Ubuntu use command

```bash
sudo apt-get install git-flow

```
## Initialise gitflow
Navigate to your repository and initialize GitFlow:

```bash
git flow init

```

## New Feature Start
To start a new feature, use the following command:
```bash
git flow feature start <feature-name>
```
This will create a new branch off the develop branch named feature/<feature-name>.


## Work on Your Feature

```bash
git add .
git commit -m "Add some feature"
```

## Finish the feature
Once you’ve completed the feature and tested it, you can finish it with:

```bash
git flow feature finish <feature-name>
```
This command will:

Merge the feature branch back into the develop branch.
Delete the feature branch.
## Push changes
After finishing your feature, push your changes to the remote repository:
```bash
git push origin develop
```


## Other gitflow commands

List Features:
```bash
git flow feature list
```
Show Feature:
```bash
git flow feature show <feature-name>
```
Publish Feature (to share your feature branch):
```bash
git flow feature publish <feature-name>
```
Track a Feature (if someone else has published it):
```bash
git flow feature track <feature-name>
```








