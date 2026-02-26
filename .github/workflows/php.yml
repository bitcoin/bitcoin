name: PHP Composer

on: [push]

jobs:
  build:

    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v1

    - name: Validate composer.json and composer.lock
      run: composer validate

    - name: Install dependencies
      run: composer install --prefer-dist --no-progress --no-suggest

    # Add a test script to composer.json, for instance: "test": "vendor/bin/phpunit"
    # Docs: https://getcomposer.org/doc/articles/scripts.md

    # - name: Run test suite
    #   run: composer run-script test
