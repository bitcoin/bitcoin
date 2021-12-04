version: '2.1'
orbs:
  node: 'circleci/node:4.1'
  slack: circleci/slack@4.1
jobs:
  deploy:
    executor:
      name: node/default
    steps:
      - checkout
      - node/install-packages
      - run:
          command: npm run deploy
      - slack/notify:
          channel: ABCXYZ
          event: fail
          template: basic_fail_1
workflows:
  deploy_and_notify:
    jobs:
      - deploy:
          context: slack-secrets
