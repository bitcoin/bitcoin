package blschia

import (
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestHashFromString(t *testing.T) {
	testCases := []struct {
		s       string
		want    Hash
		wantErr string
	}{
		{
			s: "0000000000000000000000000000000000000000000000000000000000000001",
			want: Hash{
				1, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
		},
		{
			s:       "",
			wantErr: errWrongHexStringValue.Error(),
		},
		{
			s:       "_+",
			wantErr: "invalid byte",
		},
	}
	for _, tc := range testCases {
		h, err := HashFromString(tc.s)
		assert.Equal(t, tc.want, h)
		assert.True(t, errContain(err, tc.wantErr), "got err=%v, want=%s", err, tc.wantErr)
	}
}

func errContain(err error, want string) bool {
	if err == nil && want == "" {
		return true
	}
	if err == nil && want != "" {
		return false
	}
	return strings.Contains(err.Error(), want)
}
