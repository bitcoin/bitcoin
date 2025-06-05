name: Feature Request
description: Suggest an idea for this project.
labels: [Feature]
body:
  - type: textarea
    id: feature
    attributes:
      label: Please describe the feature you'd like to see added.
      description: Attach screenshots or logs if applicable.
    validations:
      required: true
  - type: textarea
    id: related-problem
    attributes:
      label: Is your feature related to a problem, if so please describe it.
      description: Attach screenshots or logs if applicable.
    validations:
      required: false
  - type: textarea
    id: solution
    attributes:
      label: Describe the solution you'd like
    validations:
      required: false
  - type: textarea
    id: alternatives
    attributes:
      label: Describe any alternatives you've considered
    validations:
      required: false
  - type: textarea
    id: additional-context
    attributes:
      label: Please leave any additional context
    validations:
      required: false
