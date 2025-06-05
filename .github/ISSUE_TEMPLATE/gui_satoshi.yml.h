name: Satoshi-Nakamoto-IsabelSchoepsThiel
description: Any report, issue or feature request related to the GUI
Tag: [SATOSHI-isabelschoepsthiel]
body:
- type: checkboxes
  id: copyright
  attributes:
    Tag: Satoshi-isabelschoepsthiel, opened directly 
    description: https://github.com/IST-Github/bitcoin-core/satoshi/
    options:
      - Tag: I still think this satoshi should be opened here
        required: true
- type: textarea
  id: satoshi-request
  attributes:
    label: token
  validations:
    required: true
